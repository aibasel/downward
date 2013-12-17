#include "abstraction.h"

#include "labels.h"
#include "merge_and_shrink_heuristic.h" // needed for ShrinkStrategy type;
// TODO: move that type somewhere else?
#include "shrink_fh.h"

#include "../globals.h"
#include "../priority_queue.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <ext/hash_map>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>
using namespace std;
using namespace __gnu_cxx;

/* Implementation note: Transitions are grouped by their operators,
 not by source state or any such thing. Such a grouping is beneficial
 for fast generation of products because we can iterate operator by
 operator, and it also allows applying abstraction mappings very
 efficiently.

 We rarely need to be able to efficiently query the successors of a
 given state; actually, only the distance computation requires that,
 and it simply generates such a graph representation of the
 transitions itself. Various experiments have shown that maintaining
 a graph representation permanently for the benefit of distance
 computation is not worth the overhead.
 */

/*
 TODO:

 * Separate out all code related to shrinking strategies into a
 separate shrinking strategy class.

 * Try out shrinking strategies that don't care about g.

 * NOTE: We might actually also try out DFP-style strategies that don't
 just consider h, but also g. Essentially, we might set it up in such
 a way that it works with arbitrary bucketings and bucket priority
 orderings -- we'd just need to replace the current "h" components
 by bucket indices.

 * Only compute g and h when you actually need to (i.e., don't compute
 g if the strategies doesn't need it; don't compute h for the random
 shrinking strategy except for the final abstraction where we need
 it for the h value). Actually... we do need to compute g values anyway
 for the relevance test. So never mind. Although we could get rid of
 the recomputation for non-f-preserving abstractions, but that's true
 for all strategies because none of them actually needs that.

 * It's maybe a bit of a problem that by indexing the transitions
 with the *global* operator no, we have overall size requirements
 of NUM_OPS * NUM_STATE_VARS for all abstractions together. This
 is all the more unfortunate considering that we only really need
 two abstractions at the same time. Can we do something about this?

 Idea: Use an alternative indexing scheme, e.g. ones where the vector
 is indexed by some sort of "local" operator id. If we don't need
 to the lookup frequently (and I don't think we do), then we can
 use a simple hash_map<Operator *, int> for this purpose.

 * Separate the parts relating to the abstraction mapping from the
 actual abstraction, so that later during search we need not keep
 the abstractions. This would allow us to get rid of the somewhat
 ugly "release_memory" stuff.

 * Change all the terminology here so that it matches the one in
 recent papers ("merge-and-shrink", "abstraction", "transition
 system", "atomic abstraction", "product abstraction", etc.)
 Currently, "abstraction" is used both for the abstract transition
 system and for the abstraction mapping. It should only be used for the
 latter; the former maybe called "TransitionSystem" or whatever.

 */

//  TODO: We define infinity in more than a few places right now (=>
//        grep for it). It should only be defined once.
static const int infinity = numeric_limits<int>::max();

Abstraction::Abstraction(bool is_unit_cost_, Labels *labels_)
    : is_unit_cost(is_unit_cost_), labels(labels_), num_labels(labels->get_size()),
      are_labels_reduced(false), peak_memory(0) {
    clear_distances();
    // at most n-1 fresh labels will be needed if n is the number of operators
    transitions_by_label.resize(g_operators.size() * 2);
}

Abstraction::~Abstraction() {
}

string Abstraction::tag() const {
    string desc(description());
    desc[0] = toupper(desc[0]);
    return desc + ": ";
}

void Abstraction::clear_distances() {
    max_f = DISTANCE_UNKNOWN;
    max_g = DISTANCE_UNKNOWN;
    max_h = DISTANCE_UNKNOWN;
    init_distances.clear();
    goal_distances.clear();
}

int Abstraction::size() const {
    return num_states;
}

int Abstraction::get_max_f() const {
    return max_f;
}

int Abstraction::get_max_g() const {
    return max_g;
}

int Abstraction::get_max_h() const {
    return max_h;
}

void Abstraction::compute_distances() {
    cout << tag() << flush;
    if (max_h != DISTANCE_UNKNOWN) {
        cout << "distances already known" << endl;
        return;
    }

    assert(init_distances.empty() && goal_distances.empty());

    if (init_state == PRUNED_STATE) {
        cout << "init state was pruned, no distances to compute" << endl;
        // If init_state was pruned, then everything must have been pruned.
        assert(num_states == 0);
        max_f = max_g = max_h = infinity;
        return;
    }

    init_distances.resize(num_states, infinity);
    goal_distances.resize(num_states, infinity);
    if (is_unit_cost) {
        cout << "computing distances using unit-cost algorithm" << endl;
        compute_init_distances_unit_cost();
        compute_goal_distances_unit_cost();
    } else {
        cout << "computing distances using general-cost algorithm" << endl;
        compute_init_distances_general_cost();
        compute_goal_distances_general_cost();
    }

    max_f = 0;
    max_g = 0;
    max_h = 0;

    int unreachable_count = 0, irrelevant_count = 0;
    for (int i = 0; i < num_states; i++) {
        int g = init_distances[i];
        int h = goal_distances[i];
        // States that are both unreachable and irrelevant are counted
        // as unreachable, not irrelevant. (Doesn't really matter, of
        // course.)
        if (g == infinity) {
            unreachable_count++;
        } else if (h == infinity) {
            irrelevant_count++;
        } else {
            max_f = max(max_f, g + h);
            max_g = max(max_g, g);
            max_h = max(max_h, h);
        }
    }
    if (unreachable_count || irrelevant_count) {
        cout << tag()
             << "unreachable: " << unreachable_count << " states, "
             << "irrelevant: " << irrelevant_count << " states" << endl;
        /* Call shrink to discard unreachable and irrelevant states.
           The strategy must be one that prunes unreachable/irrelevant
           notes, but beyond that the details don't matter, as there
           is no need to actually shrink. So faster methods should be
           preferred. */

        /* TODO: Create a dedicated shrinking strategy from scratch,
           e.g. a bucket-based one that simply generates one good and
           one bad bucket? */

        // TODO/HACK: The way this is created is of course unspeakably
        // ugly. We'll leave this as is for now because there will likely
        // be more structural changes soon.
        ShrinkStrategy *shrink_temp = ShrinkFH::create_default(num_states);
        shrink_temp->shrink(*this, num_states, true);
        delete shrink_temp;
    }
}

int Abstraction::get_label_cost_by_index(int label_no) const {
    const Label *label = labels->get_label_by_index(label_no);
    return label->get_cost();
}

bool Abstraction::transitions_consistent() const {
    assert(num_labels <= labels->get_size());
    if (num_labels > transitions_by_label.size()) {
        return false;
    }
    for (size_t i = 0; i < transitions_by_label.size(); ++i) {
        if (i >= num_labels && !transitions_by_label[i].empty()) {
            return false;
        }
    }
    return true;
}

bool Abstraction::is_label_reduced() const {
    return (num_labels == labels->get_size());
}

int Abstraction::get_num_labels() const {
    return labels->get_size();
}

const vector<AbstractTransition> &Abstraction::get_transitions_for_label(int label_no) const {
    // we do *not* return the transitions for the mapped label because shrink_bisimulation
    // iterates over all labels anyway, otherwise transitions for some labels could be
    // dealt with several times when iterating over all labels.
    return transitions_by_label[label_no];
}

static void breadth_first_search(
    const vector<vector<int> > &graph, deque<int> &queue,
    vector<int> &distances) {
    while (!queue.empty()) {
        int state = queue.front();
        queue.pop_front();
        for (int i = 0; i < graph[state].size(); i++) {
            int successor = graph[state][i];
            if (distances[successor] > distances[state] + 1) {
                distances[successor] = distances[state] + 1;
                queue.push_back(successor);
            }
        }
    }
}

void Abstraction::compute_init_distances_unit_cost() {
    vector<vector<AbstractStateRef> > forward_graph(num_states);
    assert(transitions_consistent());
    for (int label_no = 0; label_no < num_labels; label_no++) {
        const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        for (int j = 0; j < transitions.size(); j++) {
            const AbstractTransition &trans = transitions[j];
            forward_graph[trans.src].push_back(trans.target);
        }
    }

    deque<AbstractStateRef> queue;
    for (AbstractStateRef state = 0; state < num_states; state++) {
        if (state == init_state) {
            init_distances[state] = 0;
            queue.push_back(state);
        }
    }
    breadth_first_search(forward_graph, queue, init_distances);
}

void Abstraction::compute_goal_distances_unit_cost() {
    vector<vector<AbstractStateRef> > backward_graph(num_states);
    assert(transitions_consistent());
    for (int label_no = 0; label_no < num_labels; label_no++) {
        const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        for (int j = 0; j < transitions.size(); j++) {
            const AbstractTransition &trans = transitions[j];
            backward_graph[trans.target].push_back(trans.src);
        }
    }

    deque<AbstractStateRef> queue;
    for (AbstractStateRef state = 0; state < num_states; state++) {
        if (goal_states[state]) {
            goal_distances[state] = 0;
            queue.push_back(state);
        }
    }
    breadth_first_search(backward_graph, queue, goal_distances);
}

static void dijkstra_search(
    const vector<vector<pair<int, int> > > &graph,
    AdaptiveQueue<int> &queue,
    vector<int> &distances) {
    while (!queue.empty()) {
        pair<int, int> top_pair = queue.pop();
        int distance = top_pair.first;
        int state = top_pair.second;
        int state_distance = distances[state];
        assert(state_distance <= distance);
        if (state_distance < distance)
            continue;
        for (int i = 0; i < graph[state].size(); i++) {
            const pair<int, int> &transition = graph[state][i];
            int successor = transition.first;
            int cost = transition.second;
            int successor_cost = state_distance + cost;
            if (distances[successor] > successor_cost) {
                distances[successor] = successor_cost;
                queue.push(successor_cost, successor);
            }
        }
    }
}

void Abstraction::compute_init_distances_general_cost() {
    vector<vector<pair<int, int> > > forward_graph(num_states);
    assert(transitions_consistent());
    for (int label_no = 0; label_no < num_labels; label_no++) {
        int label_cost = get_label_cost_by_index(label_no);
        const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        for (int j = 0; j < transitions.size(); j++) {
            assert(label_cost != -1);
            const AbstractTransition &trans = transitions[j];
            forward_graph[trans.src].push_back(
                make_pair(trans.target, label_cost));
        }
    }

    // TODO: Reuse the same queue for multiple computations to save speed?
    //       Also see compute_goal_distances_general_cost.
    AdaptiveQueue<int> queue;
    for (AbstractStateRef state = 0; state < num_states; state++) {
        if (state == init_state) {
            init_distances[state] = 0;
            queue.push(0, state);
        }
    }
    dijkstra_search(forward_graph, queue, init_distances);
}

void Abstraction::compute_goal_distances_general_cost() {
    vector<vector<pair<int, int> > > backward_graph(num_states);
    assert(transitions_consistent());
    for (int label_no = 0; label_no < num_labels; label_no++) {
        int label_cost = get_label_cost_by_index(label_no);
        const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        for (int j = 0; j < transitions.size(); j++) {
            assert(label_cost != -1);
            const AbstractTransition &trans = transitions[j];
            backward_graph[trans.target].push_back(
                make_pair(trans.src, label_cost));
        }
    }

    // TODO: Reuse the same queue for multiple computations to save speed?
    //       Also see compute_init_distances_general_cost.
    AdaptiveQueue<int> queue;
    for (AbstractStateRef state = 0; state < num_states; state++) {
        if (goal_states[state]) {
            goal_distances[state] = 0;
            queue.push(0, state);
        }
    }
    dijkstra_search(backward_graph, queue, goal_distances);
}

void AtomicAbstraction::apply_abstraction_to_lookup_table(
        const vector<AbstractStateRef> &abstraction_mapping) {
    cout << tag() << "applying abstraction to lookup table" << endl;
    for (int i = 0; i < lookup_table.size(); i++) {
        AbstractStateRef old_state = lookup_table[i];
        if (old_state != PRUNED_STATE)
            lookup_table[i] = abstraction_mapping[old_state];
    }
}

void CompositeAbstraction::apply_abstraction_to_lookup_table(
        const vector<AbstractStateRef> &abstraction_mapping) {
    cout << tag() << "applying abstraction to lookup table" << endl;
    for (int i = 0; i < components[0]->size(); i++) {
        for (int j = 0; j < components[1]->size(); j++) {
            AbstractStateRef old_state = lookup_table[i][j];
            if (old_state != PRUNED_STATE)
                lookup_table[i][j] = abstraction_mapping[old_state];
        }
    }
}

void Abstraction::normalize(bool reduce_labels, const EquivalenceRelation *relation) {
    // Apply label reduction and remove duplicate transitions.

    cout << tag() << "normalizing ";

    if (reduce_labels) {
        if (are_labels_reduced) {
            cout << "without label reduction (already reduced)" << endl;
        } else {
            cout << "with label reduction" << endl;
            labels->reduce_labels(relevant_labels, varset, relation);
            are_labels_reduced = true;
        }
    } else {
        cout << "without label reduction" << endl;
    }

    typedef vector<pair<AbstractStateRef, int> > StateBucket;

    /* First, partition by target state. Also replace operators by
       their canonical representatives via label reduction and clear
       away the transitions that have been processed. */
    // TODO: update comment
    vector<StateBucket> target_buckets(num_states);
    assert(transitions_consistent());
    for (int label_no = 0; label_no < num_labels; label_no++) {
        vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        if (!transitions.empty()) {
            int reduced_label_no;
            // Note: at least use the label reduction from previous label
            // reductions on other abstractions in order to make sure that the
            // new labels are used rather than the old ones.
            // We thus do *not* only use reduced labels if reduce_labels is true,
            // but always, in order to guarantee consistent labels.
            reduced_label_no = labels->get_reduced_label_no(label_no);
            for (int i = 0; i < transitions.size(); i++) {
                const AbstractTransition &t = transitions[i];
                target_buckets[t.target].push_back(
                    make_pair(t.src, reduced_label_no));
            }
            vector<AbstractTransition> ().swap(transitions);
        }
    }
    // NOTE: for the same reasons as we call "get_reduced_label_no" above even
    // if reduce_labels is false, we have to update num_labels here to guarantee
    // usage of consistent labels.
    num_labels = labels->get_size();

    // Second, partition by src state.
    vector<StateBucket> src_buckets(num_states);

    for (AbstractStateRef target = 0; target < num_states; target++) {
        StateBucket &bucket = target_buckets[target];
        for (int i = 0; i < bucket.size(); i++) {
            AbstractStateRef src = bucket[i].first;
            int label_no = bucket[i].second;
            src_buckets[src].push_back(make_pair(target, label_no));
        }
    }
    vector<StateBucket> ().swap(target_buckets);

    // Finally, partition by operator and drop duplicates.
    for (AbstractStateRef src = 0; src < num_states; src++) {
        StateBucket &bucket = src_buckets[src];
        for (int i = 0; i < bucket.size(); i++) {
            int target = bucket[i].first;
            int label_no = bucket[i].second;

            vector<AbstractTransition> &op_bucket = transitions_by_label[label_no];
            AbstractTransition trans(src, target);
            if (op_bucket.empty() || op_bucket.back() != trans)
                op_bucket.push_back(trans);
        }
    }
}

struct TransitionSignature {
    // TODO: duplicated code from LabelReducer
    vector<int> data;

    TransitionSignature(const vector<pair<int, int> > &transitions, int cost) {
        // transitions must be sorted.
        for (size_t i = 0; i < transitions.size(); ++i) {
            cout << transitions[i].first << "->" << transitions[i].second << endl;
            if (i != 0) {
                assert(transitions[i].first >= transitions[i - 1].first);
                if (transitions[i].first == transitions[i - 1].first) {
                    assert(transitions[i].second > transitions[i - 1].second);
                }
            }
            data.push_back(transitions[i].first);
            data.push_back(transitions[i].second);
        }
        data.push_back(-1); // marker
        data.push_back(cost);
    }

    bool operator==(const TransitionSignature &other) const {
        return data == other.data;
    }

    size_t hash() const {
        return ::hash_number_sequence(data, data.size());
    }
};

namespace __gnu_cxx {
template<>
struct hash<TransitionSignature> {
    size_t operator()(const TransitionSignature &sig) const {
        return sig.hash();
    }
};
}

EquivalenceRelation Abstraction::compute_local_equivalence_relation() const {
    // TODO: can we get rid of the need that the abstractions have to be
    // label reduced when computuing the label equivalence relation?
    // The problem is that when iterating over labels whose transitions have
    // not been normalized (i.e. not been "moved" to the right index and merged
    // with the transitions from all other labels of the same equivalence class),
    // we risk considering the same transitions several times.
    assert(is_label_reduced());
    //cout << (is_label_reduced() ? "" : "not ") << "label reduced" << endl;
    // groups the labels by the (uniquely ordered) set of transitions they
    // induce in the abstraction
    hash_map<TransitionSignature, vector<int> > labels_by_transitions;
    // collects all transition signatures occuring in the abstraction
    vector<TransitionSignature> transition_signatures;
    for (size_t label_no = 0; label_no < num_labels; ++label_no) {
        // NOTE: see TODO above
        if (labels->is_label_reduced(label_no))
            continue;
        const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        vector<pair<int, int> > sorted_trans;
        sorted_trans.reserve(transitions.size());
        cout << "trans for label " << label_no << endl;
        for (size_t j = 0; j < transitions.size(); ++j) {
            const AbstractTransition &trans = transitions[j];
            sorted_trans.push_back(make_pair(trans.src, trans.target));
            cout << trans.src << "->" << trans.target << endl;
        }
        ::sort(sorted_trans.begin(), sorted_trans.end());
        cout << "transition sig for label " << label_no << endl;
        TransitionSignature signature(sorted_trans, labels->get_label_by_index(label_no)->get_cost());
        if (!labels_by_transitions.count(signature)) {
            transition_signatures.push_back(signature);
        }
        labels_by_transitions[signature].push_back(label_no);
    }
    vector<pair<int, int> > labeled_label_nos;
    // TODO: we cannot use relevant_labels because even if the abstraction
    // is normalized and label reduced, relevant_labels might not be updated!
    // relevant_labels is currently never updated.
    //labeled_label_nos.reserve(relevant_labels.size());
    for (size_t i = 0; i < transition_signatures.size(); ++i) {
        const TransitionSignature &signature = transition_signatures[i];
        const vector<int> &label_nos = labels_by_transitions[signature];
        for (size_t j = 0; j < label_nos.size(); ++j) {
            int label_no = label_nos[j];
            labeled_label_nos.push_back(make_pair(i, label_no));
            /*
            // Debug: check that all grouped labels indeed have the exact
            // same set of induced transitions
            const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
            for (size_t k = j + 1; k < label_nos.size(); ++k) {
                int other_label_no = label_nos[k];
                assert(labels->get_label_by_index(label_no)->get_cost()
                       == labels->get_label_by_index(other_label_no)->get_cost());
                const vector<AbstractTransition> &other_transitions = transitions_by_label[other_label_no];
                assert(transitions.size() == other_transitions.size());
                for (size_t l = 0; l < transitions.size(); ++l) {
                    const AbstractTransition &transition = transitions[l];
                    bool matched_transition = false;
                    for (size_t m = 0; m < other_transitions.size(); ++m) {
                        const AbstractTransition &other_transition = other_transitions[m];
                        if (transition == other_transition) {
                            matched_transition = true;
                            break;
                        }
                    }
                    assert(matched_transition);
                }
            }*/
        }
    }
    // Note: with the current assumption that the abstraction is label reduced,
    // labels->get_size() equals num_labels.
    // Note: all labels not mentioned in labeled_label_nos are such labels that
    // have been mapped to new ones. They will be implicitly contained in the
    // equivalence relation and should be ignored when using it for label
    // reduction.
    return EquivalenceRelation::from_labels<int>(labels->get_size(), labeled_label_nos);
}

void Abstraction::build_atomic_abstractions(bool is_unit_cost,
    vector<Abstraction *> &result,
    Labels *labels) {
    assert(result.empty());
    cout << "Building atomic abstractions... " << endl;
    int var_count = g_variable_domain.size();

    // Step 1: Create the abstraction objects without transitions.
    for (int var_no = 0; var_no < var_count; var_no++)
        result.push_back(new AtomicAbstraction(
                             is_unit_cost, labels, var_no));

    // Step 2: Add transitions.
    // Note that when building atomic abstractions, no other labels than the
    // original operators have been added yet.
    for (int label_no = 0; label_no < labels->get_size(); label_no++) {
        const Label *label = labels->get_label_by_index(label_no);
        const vector<Prevail> &prev = label->get_prevail();
        for (int i = 0; i < prev.size(); i++) {
            int var = prev[i].var;
            int value = prev[i].prev;
            Abstraction *abs = result[var];
            AbstractTransition trans(value, value);
            abs->transitions_by_label[label_no].push_back(trans);
            if (abs->tag() == "Atomic abstraction #3: ") {
                cout << "add trans for abs " << abs->tag() << " and label " << label_no << endl;
                cout << trans.src << "->" << trans.target << endl;
            }

            if (abs->relevant_labels.empty()
                || abs->relevant_labels.back() != label)
                abs->relevant_labels.push_back(label);
        }
        const vector<PrePost> &pre_post = label->get_pre_post();
        for (int i = 0; i < pre_post.size(); i++) {
            int var = pre_post[i].var;
            int pre_value = pre_post[i].pre;
            int post_value = pre_post[i].post;
            Abstraction *abs = result[var];
            int pre_value_min, pre_value_max;
            if (pre_value == -1) {
                pre_value_min = 0;
                pre_value_max = g_variable_domain[var];
            } else {
                pre_value_min = pre_value;
                pre_value_max = pre_value + 1;
            }
            for (int value = pre_value_min; value < pre_value_max; value++) {
                AbstractTransition trans(value, post_value);
                abs->transitions_by_label[label_no].push_back(trans);
                if (abs->tag() == "Atomic abstraction #3: ") {
                    cout << "add trans for abs " << abs->tag() << " and label " << label_no << endl;
                    cout << trans.src << "->" << trans.target << endl;
                }
            }
            if (abs->relevant_labels.empty()
                || abs->relevant_labels.back() != label)
                abs->relevant_labels.push_back(label);
        }
    }
}

AtomicAbstraction::AtomicAbstraction(bool is_unit_cost, Labels *labels, int variable_)
    : Abstraction(is_unit_cost, labels), variable(variable_) {
    varset.push_back(variable);
    /*
      This generates the states of the atomic abstraction, but not the
      arcs: It is more efficient to generate all arcs of all atomic
      abstractions simultaneously.
     */
    int range = g_variable_domain[variable];

    int init_value = g_initial_state()[variable];
    int goal_value = -1;
    for (int goal_no = 0; goal_no < g_goal.size(); goal_no++) {
        if (g_goal[goal_no].first == variable) {
            assert(goal_value == -1);
            goal_value = g_goal[goal_no].second;
        }
    }

    num_states = range;
    lookup_table.reserve(range);
    goal_states.resize(num_states, false);
    for (int value = 0; value < range; value++) {
        if (value == goal_value || goal_value == -1) {
            goal_states[value] = true;
        }
        if (value == init_value)
            init_state = value;
        lookup_table.push_back(value);
    }
}

AtomicAbstraction::~AtomicAbstraction() {
}

CompositeAbstraction::CompositeAbstraction(bool is_unit_cost,
    Labels *labels, Abstraction *abs1, Abstraction *abs2)
    : Abstraction(is_unit_cost, labels) {
    cout << "Merging " << abs1->description() << " and "
         << abs2->description() << endl;

    assert(abs1->is_solvable() && abs2->is_solvable());
    assert(abs1->is_label_reduced() && abs1->is_label_reduced());

    components[0] = abs1;
    components[1] = abs2;

    ::set_union(abs1->varset.begin(), abs1->varset.end(), abs2->varset.begin(),
                abs2->varset.end(), back_inserter(varset));

    num_states = abs1->size() * abs2->size();
    goal_states.resize(num_states, false);

    lookup_table.resize(abs1->size(), vector<AbstractStateRef> (abs2->size()));
    for (int s1 = 0; s1 < abs1->size(); s1++) {
        for (int s2 = 0; s2 < abs2->size(); s2++) {
            int state = s1 * abs2->size() + s2;
            lookup_table[s1][s2] = state;
            if (abs1->goal_states[s1] && abs2->goal_states[s2])
                goal_states[state] = true;
            if (s1 == abs1->init_state && s2 == abs2->init_state)
                init_state = state;
        }
    }

    for (int i = 0; i < abs1->relevant_labels.size(); i++)
        abs1->relevant_labels[i]->get_reduced_label()->marker1 = true;
    for (int i = 0; i < abs2->relevant_labels.size(); i++)
        abs2->relevant_labels[i]->get_reduced_label()->marker2 = true;

    int multiplier = abs2->size();
    for (int label_no = 0; label_no < num_labels; label_no++) {
        const Label *label = labels->get_label_by_index(label_no);
        bool relevant1 = label->marker1;
        bool relevant2 = label->marker2;
        if (relevant1 || relevant2) {
            vector<AbstractTransition> &transitions = transitions_by_label[label_no];
            relevant_labels.push_back(label);
            const vector<AbstractTransition> &bucket1 =
                abs1->transitions_by_label[label_no];
            const vector<AbstractTransition> &bucket2 =
                abs2->transitions_by_label[label_no];
            if (relevant1 && relevant2) {
                transitions.reserve(bucket1.size() * bucket2.size());
                for (int i = 0; i < bucket1.size(); i++) {
                    int src1 = bucket1[i].src;
                    int target1 = bucket1[i].target;
                    for (int j = 0; j < bucket2.size(); j++) {
                        int src2 = bucket2[j].src;
                        int target2 = bucket2[j].target;
                        int src = src1 * multiplier + src2;
                        int target = target1 * multiplier + target2;
                        transitions.push_back(AbstractTransition(src, target));
                    }
                }
            } else if (relevant1) {
                assert(!relevant2);
                transitions.reserve(bucket1.size() * abs2->size());
                for (int i = 0; i < bucket1.size(); i++) {
                    int src1 = bucket1[i].src;
                    int target1 = bucket1[i].target;
                    for (int s2 = 0; s2 < abs2->size(); s2++) {
                        int src = src1 * multiplier + s2;
                        int target = target1 * multiplier + s2;
                        transitions.push_back(AbstractTransition(src, target));
                    }
                }
            } else if (relevant2) {
                assert(!relevant1);
                transitions.reserve(bucket2.size() * abs1->size());
                for (int i = 0; i < bucket2.size(); i++) {
                    int src2 = bucket2[i].src;
                    int target2 = bucket2[i].target;
                    for (int s1 = 0; s1 < abs1->size(); s1++) {
                        int src = s1 * multiplier + src2;
                        int target = s1 * multiplier + target2;
                        transitions.push_back(AbstractTransition(src, target));
                    }
                }
            }
        }
    }

    for (int i = 0; i < abs1->relevant_labels.size(); i++)
        abs1->relevant_labels[i]->get_reduced_label()->marker1 = false;
    for (int i = 0; i < abs2->relevant_labels.size(); i++)
        abs2->relevant_labels[i]->get_reduced_label()->marker2 = false;
}

CompositeAbstraction::~CompositeAbstraction() {
}

string AtomicAbstraction::description() const {
    ostringstream s;
    s << "atomic abstraction #" << variable;
    return s.str();
}

string CompositeAbstraction::description() const {
    ostringstream s;
    s << "abstraction (" << varset.size() << "/"
      << g_variable_domain.size() << " vars)";
    return s.str();
}

AbstractStateRef AtomicAbstraction::get_abstract_state(const State &state) const {
    int value = state[variable];
    return lookup_table[value];
}

AbstractStateRef CompositeAbstraction::get_abstract_state(const State &state) const {
    AbstractStateRef state1 = components[0]->get_abstract_state(state);
    AbstractStateRef state2 = components[1]->get_abstract_state(state);
    if (state1 == PRUNED_STATE || state2 == PRUNED_STATE)
        return PRUNED_STATE;
    return lookup_table[state1][state2];
}

void Abstraction::apply_abstraction(
    vector<slist<AbstractStateRef> > &collapsed_groups) {
    /* Note on how this method interacts with the distance information
       (init_distances and goal_distances): if no two states with
       different g or h values are combined by the abstraction (i.e.,
       if the abstraction is "f-preserving", then this method makes
       sure sure that distance information is preserved.

       This is important because one of the (indirect) callers of this
       method is the distance computation code, which uses it in a
       somewhat roundabout way to get rid of irrelevant and
       unreachable states. That caller will always give us an
       f-preserving abstraction.

       When called with a non-f-preserving abstraction, distance
       information is cleared as a side effect. In most cases we won't
       actually need it any more at this point anyway, so it is no
       great loss.

       Still, it might be good if we could find a way to perform the
       unreachability and relevance pruning that didn't introduce such
       tight coupling between the distance computation and abstraction
       code. It would probably also a good idea to do the
       unreachability/relevance pruning as early as possible, e.g.
       right after construction.
     */

    cout << tag() << "applying abstraction (" << size()
         << " to " << collapsed_groups.size() << " states)" << endl;

    typedef slist<AbstractStateRef> Group;

    vector<int> abstraction_mapping(num_states, PRUNED_STATE);

    for (int group_no = 0; group_no < collapsed_groups.size(); group_no++) {
        Group &group = collapsed_groups[group_no];
        for (Group::iterator pos = group.begin(); pos != group.end(); ++pos) {
            AbstractStateRef state = *pos;
            assert(abstraction_mapping[state] == PRUNED_STATE);
            abstraction_mapping[state] = group_no;
        }
    }

    int new_num_states = collapsed_groups.size();
    vector<int> new_init_distances(new_num_states, infinity);
    vector<int> new_goal_distances(new_num_states, infinity);
    vector<bool> new_goal_states(new_num_states, false);

    bool must_clear_distances = false;
    for (AbstractStateRef new_state = 0; new_state < collapsed_groups.size(); new_state++) {
        Group &group = collapsed_groups[new_state];
        assert(!group.empty());

        Group::iterator pos = group.begin();
        int &new_init_dist = new_init_distances[new_state];
        int &new_goal_dist = new_goal_distances[new_state];

        new_init_dist = init_distances[*pos];
        new_goal_dist = goal_distances[*pos];
        new_goal_states[new_state] = goal_states[*pos];

        ++pos;
        for (; pos != group.end(); ++pos) {
            if (init_distances[*pos] < new_init_dist) {
                must_clear_distances = true;
                new_init_dist = init_distances[*pos];
            }
            if (goal_distances[*pos] < new_goal_dist) {
                must_clear_distances = true;
                new_goal_dist = goal_distances[*pos];
            }
            if (goal_states[*pos])
                new_goal_states[new_state] = true;
        }
    }

    // Release memory.
    vector<int>().swap(init_distances);
    vector<int>().swap(goal_distances);
    vector<bool>().swap(goal_states);

    if (tag() == "Atomic abstraction #3: ") {
        cout << "trans by label before shrink:" << endl;
        for (size_t i = 0; i < transitions_by_label.size(); ++i) {
            const vector<AbstractTransition> &transitions = transitions_by_label[i];
            cout << "trans for abs " << tag() << " and label " << i << endl;
            for (size_t j = 0; j < transitions.size(); ++j) {
                cout << abstraction_mapping[transitions[j].src] << "->" << abstraction_mapping[transitions[j].target] << endl;
            }
        }
    }

    vector<vector<AbstractTransition> > new_transitions_by_label(
        transitions_by_label.size());
    assert(transitions_consistent());
    for (int label_no = 0; label_no < num_labels; label_no++) {
        // TODO: check correctness! What happens if several labels are mapped
        // to the same label and thus to the same "new_transitions" (which then
        // is possibly non-empty when trying to reserve space...)?
        const vector<AbstractTransition> &transitions =
            transitions_by_label[label_no];
        int mapped_label_no = labels->get_reduced_label_no(label_no);
        // TODO: check that mapped_label_no == label_no iff ...?
        vector<AbstractTransition> &new_transitions =
            new_transitions_by_label[mapped_label_no];
        new_transitions.reserve(new_transitions.size() + transitions.size());
        for (int i = 0; i < transitions.size(); i++) {
            const AbstractTransition &trans = transitions[i];
            int src = abstraction_mapping[trans.src];
            int target = abstraction_mapping[trans.target];
            if (src != PRUNED_STATE && target != PRUNED_STATE)
                new_transitions.push_back(AbstractTransition(src, target));
        }
    }
    vector<vector<AbstractTransition> > ().swap(transitions_by_label);
    // need to update num_labels here as the abstraction is now "up to date"
    // in terms of all labels are correctly mapped to their new labels.
    num_labels = labels->get_size();

    num_states = new_num_states;
    transitions_by_label.swap(new_transitions_by_label);
    init_distances.swap(new_init_distances);
    goal_distances.swap(new_goal_distances);
    goal_states.swap(new_goal_states);
    init_state = abstraction_mapping[init_state];
    if (init_state == PRUNED_STATE)
        cout << tag() << "initial state pruned; task unsolvable" << endl;

    apply_abstraction_to_lookup_table(abstraction_mapping);

    if (must_clear_distances) {
        cout << tag() << "simplification was not f-preserving!" << endl;
        clear_distances();
    }

    if (tag() == "Atomic abstraction #3: ") {
        cout << "trans by label after shrink:" << endl;
        for (size_t i = 0; i < transitions_by_label.size(); ++i) {
            const vector<AbstractTransition> &transitions = transitions_by_label[i];
            cout << "trans for abs " << tag() << " and label " << i << endl;
            for (size_t j = 0; j < transitions.size(); ++j) {
                cout << transitions[j].src << "->" << transitions[j].target << endl;
            }
        }
    }
}

bool Abstraction::is_solvable() const {
    return init_state != PRUNED_STATE;
}

int Abstraction::get_cost(const State &state) const {
    int abs_state = get_abstract_state(state);
    if (abs_state == PRUNED_STATE)
        return -1;
    int cost = goal_distances[abs_state];
    assert(cost != infinity);
    return cost;
}

int Abstraction::memory_estimate() const {
    int result = sizeof(Abstraction);
    result += sizeof(Operator *) * relevant_labels.capacity();
    result += sizeof(vector<AbstractTransition> )
              * transitions_by_label.capacity();
    for (int i = 0; i < transitions_by_label.size(); i++)
        result += sizeof(AbstractTransition) * transitions_by_label[i].capacity();
    result += sizeof(int) * init_distances.capacity();
    result += sizeof(int) * goal_distances.capacity();
    result += sizeof(bool) * goal_states.capacity();
    return result;
}

int AtomicAbstraction::memory_estimate() const {
    int result = Abstraction::memory_estimate();
    result += sizeof(AtomicAbstraction) - sizeof(Abstraction);
    result += sizeof(AbstractStateRef) * lookup_table.capacity();
    return result;
}

int CompositeAbstraction::memory_estimate() const {
    int result = Abstraction::memory_estimate();
    result += sizeof(CompositeAbstraction) - sizeof(Abstraction);
    result += sizeof(vector<AbstractStateRef> ) * lookup_table.capacity();
    for (int i = 0; i < lookup_table.size(); i++)
        result += sizeof(AbstractStateRef) * lookup_table[i].capacity();
    return result;
}

void Abstraction::release_memory() {
    vector<const Label *>().swap(relevant_labels);
    vector<vector<AbstractTransition> >().swap(transitions_by_label);
}

int Abstraction::total_transitions() const {
    int total = 0;
    for (int i = 0; i < transitions_by_label.size(); i++)
        total += transitions_by_label[i].size();
    return total;
}

int Abstraction::unique_unlabeled_transitions() const {
    vector<AbstractTransition> unique_transitions;
    for (int i = 0; i < transitions_by_label.size(); i++) {
        const vector<AbstractTransition> &trans = transitions_by_label[i];
        unique_transitions.insert(unique_transitions.end(), trans.begin(),
                                  trans.end());
    }
    ::sort(unique_transitions.begin(), unique_transitions.end());
    return unique(unique_transitions.begin(), unique_transitions.end())
           - unique_transitions.begin();
}

void Abstraction::statistics(bool include_expensive_statistics) const {
    int memory = memory_estimate();
    peak_memory = max(peak_memory, memory);
    cout << tag() << size() << " states, ";
    if (include_expensive_statistics)
        cout << unique_unlabeled_transitions();
    else
        cout << "???";
    cout << "/" << total_transitions() << " arcs, " << memory << " bytes"
         << endl;
    cout << tag();
    if (max_h == DISTANCE_UNKNOWN) {
        cout << "distances not computed";
    } else if (is_solvable()) {
        cout << "init h=" << goal_distances[init_state] << ", max f=" << max_f
             << ", max g=" << max_g << ", max h=" << max_h;
    } else {
        cout << "abstraction is unsolvable";
    }
    cout << " [t=" << g_timer << "]" << endl;
}

int Abstraction::get_peak_memory_estimate() const {
    return peak_memory;
}

bool Abstraction::is_in_varset(int var) const {
    return find(varset.begin(), varset.end(), var) != varset.end();
}

void Abstraction::dump() const {
    cout << "digraph abstract_transition_graph";
    for (int i = 0; i < varset.size(); i++)
        cout << "_" << varset[i];
    cout << " {" << endl;
    cout << "    node [shape = none] start;" << endl;
    for (int i = 0; i < num_states; i++) {
        bool is_init = (i == init_state);
        bool is_goal = (goal_states[i] == true);
        cout << "    node [shape = " << (is_goal ? "doublecircle" : "circle")
             << "] node" << i << ";" << endl;
        if (is_init)
            cout << "    start -> node" << i << ";" << endl;
    }
    // TODO: assertion invalidated by new Label Class
    //assert(transitions_by_label.size() == g_operators.size());
    for (int label_no = 0; label_no < transitions_by_label.size(); label_no++) {
        const vector<AbstractTransition> &trans = transitions_by_label[label_no];
        for (int i = 0; i < trans.size(); i++) {
            int src = trans[i].src;
            int target = trans[i].target;
            cout << "    node" << src << " -> node" << target << " [label = o_"
                 << label_no << "];" << endl;
        }
    }
    cout << "}" << endl;
}
