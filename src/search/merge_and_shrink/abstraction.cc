#include "abstraction.h"

#include "label.h"
#include "labels.h"
#include "shrink_fh.h"

#include "../equivalence_relation.h"
#include "../globals.h"
#include "../priority_queue.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <fstream>
#include <limits>
#include <set>
#include <string>
#include <sstream>
using namespace std;
using namespace __gnu_cxx;

/* Implementation note: Transitions are grouped by their labels,
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

//  TODO: We define infinity in more than a few places right now (=>
//        grep for it). It should only be defined once.
static const int infinity = numeric_limits<int>::max();

Abstraction::Abstraction(Labels *labels_)
    : labels(labels_), num_labels(labels->get_size()),
      transitions_by_label(g_operators.empty() ? 0 : g_operators.size() * 2 - 1),
      relevant_labels(transitions_by_label.size(), false),
      transitions_sorted_unique(true), peak_memory(0) {
    clear_distances();
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

int Abstraction::get_label_cost_by_index(int label_no) const {
    const Label *label = labels->get_label_by_index(label_no);
    return label->get_cost();
}

const vector<AbstractTransition> &Abstraction::get_transitions_for_label(int label_no) const {
    return transitions_by_label[label_no];
}

int Abstraction::get_num_labels() const {
    return labels->get_size();
}

void Abstraction::compute_label_ranks(vector<int> &label_ranks) {
    // abstraction needs to be normalized when considering labels and their
    // transitions
    if (!is_normalized()) {
        normalize();
    }
    // distances must be computed
    if (max_h == DISTANCE_UNKNOWN) {
        compute_distances();
    }
    assert(label_ranks.empty());
    label_ranks.reserve(transitions_by_label.size());
    for (size_t label_no = 0; label_no < transitions_by_label.size(); ++label_no) {
        if (relevant_labels[label_no]) {
            const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
            int label_rank = infinity;
            for (size_t i = 0; i < transitions.size(); ++i) {
                const AbstractTransition &t = transitions[i];
                label_rank = min(label_rank, goal_distances[t.target]);
            }
            // relevant labels with no transitions have a rank of infinity (they
            // block snychronization)
            label_ranks.push_back(label_rank);
        } else {
            label_ranks.push_back(-1);
        }
    }
}

bool Abstraction::are_distances_computed() const {
    if (max_h == DISTANCE_UNKNOWN ) {
        assert(max_f == DISTANCE_UNKNOWN);
        assert(max_g == DISTANCE_UNKNOWN);
        assert(init_distances.empty());
        assert(goal_distances.empty());
        return false;
    }
    return true;
}

void Abstraction::compute_distances() {
    cout << tag() << flush;
    if (are_distances_computed()) {
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
    if (labels->is_unit_cost()) {
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
    for (int label_no = 0; label_no < num_labels; label_no++) {
        int label_cost = get_label_cost_by_index(label_no);
        const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        for (int j = 0; j < transitions.size(); j++) {
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
    for (int label_no = 0; label_no < num_labels; label_no++) {
        int label_cost = get_label_cost_by_index(label_no);
        const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        for (int j = 0; j < transitions.size(); j++) {
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

bool Abstraction::are_transitions_sorted_unique() const {
    for (size_t i = 0; i < transitions_by_label.size(); ++i) {
        const vector<AbstractTransition> &transitions = transitions_by_label[i];
        if (!is_sorted_unique(transitions)) {
            return false;
        }
    }
    return true;
}

bool Abstraction::is_normalized() const {
    return ((num_labels == labels->get_size()) && transitions_sorted_unique);
}

void Abstraction::normalize() {
    /* This method sorts all transitions and removes duplicate transitions.
       It also maps the labels so that they are up to date with the labels
       object. */

    if (is_normalized()) {
        return;
    }
    //cout << tag() << "normalizing" << endl;

    typedef vector<pair<AbstractStateRef, int> > StateBucket;

    /* First, partition by target state. Possibly replace labels by
       their new label which they are mapped to via label reduction and clear
       away the transitions that have been processed. */
    vector<StateBucket> target_buckets(num_states);

    /* We handle "old" and "new" labels separately. The following loop
       goes over all labels that were already known when we last
       normalized. It ignores labels that are no longer current
       because they have been mapped to fresh labels in the last label
       reduction. (These labels are handled separately in the next
       loop.) */

    for (int label_no = 0; label_no < num_labels; label_no++) {
        if (labels->is_label_reduced(label_no)) {
            // skip all labels that have been reduced (previously, in which case
            // they do not induce any transitions or recently, in which case we
            // deal with them separately).
            continue;
        }
        vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        for (int i = 0; i < transitions.size(); i++) {
            const AbstractTransition &t = transitions[i];
            target_buckets[t.target].push_back(
                make_pair(t.src, label_no));
        }
        vector<AbstractTransition> ().swap(transitions);
    }

    /* Now we handle "new" labels. We iterate over the fresh labels and
       determine the "old" labels from which they were generated to combine
       their transitions.

       Some complications arise when we combine labels of which some
       are relevant and others are not. In this case, we test if all
       transitions of relevant labels are self-loops. If this happens,
       we make the new label irrelevant. Otherwise, the new labels is
       relevant and we must materialize all previously implicit
       self-loops.

       Note that currently we do not detect the case where a label
       becomes irrelevant due to shrinking. This could be a future
       optimization.
    */

    /* labels_made_irrelevant stores labels for which we collect
       transitions that later turn out to be unnecessary because the
       label becomes irrelevant.
    */
    hash_set<int> labels_made_irrelevant;
    for (int reduced_label_no = num_labels; reduced_label_no < labels->get_size();
         ++reduced_label_no) {
        const Label *reduced_label = labels->get_label_by_index(reduced_label_no);
        const vector<Label *> &parents = reduced_label->get_parents();
        bool some_parent_is_irrelevant = false;
        bool all_transitions_are_self_loops = true;
        for (size_t i = 0; i < parents.size(); ++i) {
            const Label *parent = parents[i];
            int parent_id = parent->get_id();
            // We require that we only have to deal with one label reduction at
            // at time when normalizing. Otherwise the following assertion could
            // be broken and we would need to consider parents' parents and so
            // on...
            assert(parent_id < num_labels);
            vector<AbstractTransition> &transitions =
                    transitions_by_label[parent_id];

            if (relevant_labels[parent_id]) {
                for (int i = 0; i < transitions.size(); i++) {
                    const AbstractTransition &t = transitions[i];
                    target_buckets[t.target].push_back(
                        make_pair(t.src, reduced_label_no));
                    if (t.target != t.src) {
                        all_transitions_are_self_loops = false;
                    }
                }
                vector<AbstractTransition> ().swap(transitions);

                // mark parent as irrelevant (unused labels should not be
                // marked as relevant in order to avoid confusions when
                // considering all relevant labels).
                relevant_labels[parent_id] = false;
            } else {
                some_parent_is_irrelevant = true;
            }
        }
        if (some_parent_is_irrelevant) {
            if (all_transitions_are_self_loops) {
                // new label is irrelevant (implicit self-loops)
                // remove all transitions (later)
                labels_made_irrelevant.insert(reduced_label_no);
            } else {
                // new label is relevant
                relevant_labels[reduced_label_no] = true;
                // make self loops explicit
                for (int i = 0; i < num_states; ++i) {
                    target_buckets[i].push_back(
                        make_pair(i, reduced_label_no));
                }
            }
        } else {
            // new label is relevant
            relevant_labels[reduced_label_no] = true;
        }
    }

    // Second, partition by src state.
    vector<StateBucket> src_buckets(num_states);

    for (AbstractStateRef target = 0; target < num_states; target++) {
        StateBucket &bucket = target_buckets[target];
        for (int i = 0; i < bucket.size(); i++) {
            AbstractStateRef src = bucket[i].first;
            int label_no = bucket[i].second;
            if (labels_made_irrelevant.count(label_no)) {
                assert(transitions_by_label[label_no].empty());
            } else {
                src_buckets[src].push_back(make_pair(target, label_no));
            }
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

    // Abstraction has been normalized, restore invariant
    assert(are_transitions_sorted_unique());
    num_labels = labels->get_size();
    transitions_sorted_unique = true;
    assert(is_normalized());
}

EquivalenceRelation *Abstraction::compute_local_equivalence_relation() const {
    /* If label l1 is irrelevant and label l2 is relevant but has exactly the
       transitions of an irrelevant label, we do not detect the equivalence. */

    assert(is_normalized());
    vector<bool> considered_labels(num_labels, false);
    vector<pair<int, int> > annotated_labels;
    int annotation = 0;
    for (size_t label_no = 0; label_no < num_labels; ++label_no) {
        if (labels->is_label_reduced(label_no)) {
            // do not consider non-leaf labels
            continue;
        }
        if (considered_labels[label_no]) {
            continue;
        }
        int label_cost = get_label_cost_by_index(label_no);
        annotated_labels.push_back(make_pair(annotation, label_no));
        const vector<AbstractTransition> &transitions = transitions_by_label[label_no];
        for (size_t other_label_no = label_no + 1; other_label_no < num_labels; ++other_label_no) {
            if (labels->is_label_reduced(other_label_no)) {
                // do not consider non-leaf labels
                continue;
            }
            if (considered_labels[other_label_no]) {
                continue;
            }
            if (label_cost != get_label_cost_by_index(other_label_no)) {
                continue;
            }
            if (relevant_labels[label_no] != relevant_labels[other_label_no]) {
                continue;
            }
            const vector<AbstractTransition> &other_transitions = transitions_by_label[other_label_no];
            if ((transitions.empty() && other_transitions.empty())
                || (transitions == other_transitions)) {
                considered_labels[other_label_no] = true;
                annotated_labels.push_back(make_pair(annotation, other_label_no));
            }
        }
        ++annotation;
    }
    return EquivalenceRelation::from_annotated_elements<int>(num_labels, annotated_labels);
}

void Abstraction::build_atomic_abstractions(vector<Abstraction *> &result,
                                            Labels *labels) {
    assert(result.empty());
    cout << "Building atomic abstractions... " << endl;
    int var_count = g_variable_domain.size();

    // Step 1: Create the abstraction objects without transitions.
    for (int var_no = 0; var_no < var_count; var_no++)
        result.push_back(new AtomicAbstraction(labels, var_no));

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
            abs->relevant_labels[label_no] = true;
        }
        const vector<PrePost> &pre_post = label->get_pre_post();
        for (int i = 0; i < pre_post.size(); i++) {
            int var = pre_post[i].var;
            int post_value = pre_post[i].post;
            Abstraction *abs = result[var];

            // Determine possible values that var can have when this
            // operator is applicable.
            int pre_value = pre_post[i].pre;
            int pre_value_min, pre_value_max;
            if (pre_value == -1) {
                pre_value_min = 0;
                pre_value_max = g_variable_domain[var];
            } else {
                pre_value_min = pre_value;
                pre_value_max = pre_value + 1;
            }

            // cond_effect_pre_value == x means that the effect has an
            // effect condition "var == x".
            // cond_effect_pre_value == -1 means no effect condition on var.
            // has_other_effect_cond is true iff there exists an effect
            // condition on a variable other than var.
            const vector<Prevail> &eff_cond = pre_post[i].cond;
            int cond_effect_pre_value = -1;
            bool has_other_effect_cond = false;
            for (size_t j = 0; j < eff_cond.size(); ++j) {
                if (eff_cond[j].var == var) {
                    cond_effect_pre_value = eff_cond[j].prev;
                } else {
                    has_other_effect_cond = true;
                }
            }

            // Handle transitions that occur when the effect triggers.
            for (int value = pre_value_min; value < pre_value_max; value++) {
                /* Only add a transition if it is possible that the effect
                   triggers. We can rule out that the effect triggers if it has
                   a condition on var and this condition is not satisfied. */
                if (cond_effect_pre_value == -1 || cond_effect_pre_value == value) {
                    AbstractTransition trans(value, post_value);
                    abs->transitions_by_label[label_no].push_back(trans);
                }
            }

            // Handle transitions that occur when the effect does not trigger.
            if (!eff_cond.empty()) {
                for (int value = pre_value_min; value < pre_value_max; value++) {
                    /* Add self-loop if the effect might not trigger.
                       If the effect has a condition on another variable, then
                       it can fail to trigger no matter which value var has.
                       If it only has a condition on var, then the effect
                       fails to trigger if this condition is false. */
                    if (has_other_effect_cond || value != cond_effect_pre_value) {
                        AbstractTransition loop(value, value);
                        abs->transitions_by_label[label_no].push_back(loop);
                    }
                }
            }

            abs->relevant_labels[label_no] = true;
        }
    }

    for (size_t i = 0; i < result.size(); ++i) {
        assert(result[i]->are_transitions_sorted_unique());
        assert(result[i]->is_normalized());
    }
}

AtomicAbstraction::AtomicAbstraction(Labels *labels, int variable_)
    : Abstraction(labels), variable(variable_) {
    varset.push_back(variable);
    /*
      This generates the states of the atomic abstraction, but not the
      arcs: It is more efficient to generate all arcs of all atomic
      abstractions simultaneously.
     */
    int range = g_variable_domain[variable];

    int init_value = g_initial_state()[variable];
    int goal_value = -1;
    goal_relevant = false;
    for (int goal_no = 0; goal_no < g_goal.size(); goal_no++) {
        if (g_goal[goal_no].first == variable) {
            goal_relevant = true;
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

CompositeAbstraction::CompositeAbstraction(Labels *labels,
                                           Abstraction *abs1,
                                           Abstraction *abs2)
    : Abstraction(labels) {
    cout << "Merging " << abs1->description() << " and "
         << abs2->description() << endl;

    assert(abs1->is_solvable() && abs2->is_solvable());
    assert(abs1->is_normalized() && abs2->is_normalized());

    components[0] = abs1;
    components[1] = abs2;

    ::set_union(abs1->varset.begin(), abs1->varset.end(), abs2->varset.begin(),
                abs2->varset.end(), back_inserter(varset));

    num_states = abs1->size() * abs2->size();
    goal_states.resize(num_states, false);
    goal_relevant = (abs1->goal_relevant || abs2->goal_relevant);

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

    /* Note:
       The way we construct the transitions of the new composite abstraction,
       we cannot easily guarantee that they are sorted. Given that we have
       transitions a->b and c->d in abstraction one and two, we would like to
       have (a,c)->(b,d) in the resultin abstraction. There is no obvious way
       at looking at the transitions of abstractions one and two that would
       result in the desired ordering. Even in the case that the second
       abstractions has only self loops, this is not trivial, as we would like
       to sort transitions to (a,c,b). Only in the case that the first
       abstraction has only self-lopos, by looking at each transition of the
       first abstraction and multiplying in out with the transitions of the
       second transition, we obtain the desired order (a,c,d).
     */
    int multiplier = abs2->size();
    for (int label_no = 0; label_no < num_labels; label_no++) {
        bool relevant1 = abs1->relevant_labels[label_no];
        bool relevant2 = abs2->relevant_labels[label_no];
        if (relevant1 || relevant2) {
            relevant_labels[label_no] = true;
            vector<AbstractTransition> &transitions = transitions_by_label[label_no];
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
                for (int s1 = 0; s1 < abs1->size(); s1++) {
                    for (int i = 0; i < bucket2.size(); i++) {
                        int src2 = bucket2[i].src;
                        int target2 = bucket2[i].target;
                        int src = s1 * multiplier + src2;
                        int target = s1 * multiplier + target2;
                        transitions.push_back(AbstractTransition(src, target));
                    }
                }
                assert(is_sorted_unique(transitions));
            }
        }
    }

    // TODO do not check if transitions are sorted but just assume they are not?
    if (!are_transitions_sorted_unique())
        transitions_sorted_unique = false;
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

    // abstraction must have been normalized if any labels have been reduced
    // before. Note that we do *not* require transitions to be sorted (thus
    // not asserting is_normalized()), because shrink can indirectly be called
    // from the distances computation, which is done for the final abstraction
    // which on its turn may not be normalized at that time.
    assert(num_labels == labels->get_size());
    // distances must have been computed before
    assert(are_distances_computed());

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
            if (init_distances[*pos] != new_init_dist) {
                must_clear_distances = true;
            }
            if (goal_distances[*pos] != new_goal_dist) {
                must_clear_distances = true;
            }
            if (goal_states[*pos])
                new_goal_states[new_state] = true;
        }
    }

    // Release memory.
    vector<int>().swap(init_distances);
    vector<int>().swap(goal_distances);
    vector<bool>().swap(goal_states);

    vector<vector<AbstractTransition> > new_transitions_by_label(
        transitions_by_label.size());
    for (int label_no = 0; label_no < num_labels; label_no++) {
        if (labels->is_label_reduced(label_no)) {
            // do not consider non-leaf labels
            continue;
        }
        const vector<AbstractTransition> &transitions =
            transitions_by_label[label_no];
        vector<AbstractTransition> &new_transitions =
            new_transitions_by_label[label_no];
        new_transitions.reserve(transitions.size());
        for (int i = 0; i < transitions.size(); i++) {
            const AbstractTransition &trans = transitions[i];
            int src = abstraction_mapping[trans.src];
            int target = abstraction_mapping[trans.target];
            if (src != PRUNED_STATE && target != PRUNED_STATE)
                new_transitions.push_back(AbstractTransition(src, target));
        }
    }
    vector<vector<AbstractTransition> > ().swap(transitions_by_label);

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

    // TODO do not check if transitions are sorted but just assume they are not?
    if (!are_transitions_sorted_unique())
        transitions_sorted_unique = false;
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
    result += sizeof(Label *) * relevant_labels.capacity();
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
    vector<bool>().swap(relevant_labels);
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
    if (!are_distances_computed()) {
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

void Abstraction::dump_relevant_labels() const {
    cout << "relevant labels" << endl;
    for (size_t label_no = 0; label_no < relevant_labels.size(); ++label_no) {
        if (label_no) {
            cout << label_no << endl;
        }
    }
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
    for (int label_no = 0; label_no < num_labels; label_no++) {
        // reduced labels are automatically skipped because trans is then empty
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
