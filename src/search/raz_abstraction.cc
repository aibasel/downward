#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <deque>
#include <iostream>
using namespace std;

#include "raz_abstraction.h"
#include "raz_mas_heuristic.h" // needed for ShrinkStrategy type;
// TODO: move that type somewhere else?
#include "globals.h"
#include "operator.h"
#include "raz_operator_registry.h"
#include "timer.h"

/* Implementation note: Transitions are grouped by their operators,
 not by source node or any such thing. Such a grouping is beneficial
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
 former; the latter maybe called "TransitionSystem" or whatever.

 */

inline int get_op_index(const Operator *op) {
    /* TODO: The op_index computation is duplicated from
     OperatorRegistry::get_op_index() and actually belongs neither
     here nor there. There should be some canonical way of getting
     from an Operator pointer to an index, but it's not clear how to
     do this in a way that best fits the overall planner
     architecture (taking into account axioms etc.) */
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < g_operators.size());
    return op_index;
}

Abstraction::Abstraction()
    : peak_memory(0) {
    transitions_by_op.resize(g_operators.size());
    //	reduced_to_by_op.resize(g_operators.size());
}

Abstraction::~Abstraction() {
}

int Abstraction::size() const {
    return num_states;
}

void Abstraction::compute_distances() {
    compute_init_distances();
    compute_goal_distances();

    max_f = 0;
    max_g = 0;
    max_h = 0;

    int unreachable_count = 0, irrelevant_count = 0;
    for (int i = 0; i < num_states; i++) {
        int g = init_distances[i];
        int h = goal_distances[i];
        if (g == QUITE_A_LOT) {
            unreachable_count++;
        } else if (h == QUITE_A_LOT) {
            irrelevant_count++;
        } else {
            max_f = max(max_f, g + h);
            max_g = max(max_g, g);
            max_h = max(max_h, h);
        }
    }
    if (unreachable_count || irrelevant_count) {
        cout << "unreachable: " << unreachable_count << " nodes, "
             << "irrelevant: " << irrelevant_count << " nodes" << endl;
        // Call shrink to discard unreachable and irrelevant states.
        // The exact strategy doesn't matter much (although it should
        // be efficient) as there is no need to actually shrink.
        //TODO - very important - since no other shrinking strategy supposrt AC yet, I put SHRINK_BISIMULATION_NO_MEMORY_LIMIT...
        shrink(1, SHRINK_BISIMULATION_NO_MEMORY_LIMIT, true);
        //shrink(size(), SHRINK_HIGH_F_LOW_H, true);
    }
}

static void breadth_first_search(const vector<vector<int> > &graph,
                                 const vector<vector<int> > &cost_graph, deque<int> &queue,
                                 vector<int> &distances) {
    while (!queue.empty()) {
        int state = queue.front();
        queue.pop_front();
        for (int i = 0; i < graph[state].size(); i++) {
            int successor = graph[state][i];
            int cost = cost_graph[state][i];
            if (distances[successor] > distances[state] + cost) {
                distances[successor] = distances[state] + cost;
                queue.push_back(successor);
            }
        }
    }
}

void Abstraction::compute_init_distances() {
    vector<vector<AbstractStateRef> > forward_graph(num_states);
    vector<vector<int> > forward_graph_costs(num_states);
    for (int i = 0; i < transitions_by_op.size(); i++) {
        const vector<AbstractTransition> &transitions = transitions_by_op[i];
        for (int j = 0; j < transitions.size(); j++) {
            const AbstractTransition &trans = transitions[j];
            forward_graph[trans.src].push_back(trans.target);
            forward_graph_costs[trans.src].push_back(trans.cost);
        }
    }

    deque<AbstractStateRef> queue;
    for (AbstractStateRef state = 0; state < num_states; state++) {
        if (state == init_state) {
            init_distances[state] = 0;
            queue.push_back(state);
        } else {
            init_distances[state] = QUITE_A_LOT;
        }
    }
    breadth_first_search(forward_graph, forward_graph_costs, queue,
                         init_distances);
}

void Abstraction::compute_goal_distances() {
    vector<vector<AbstractStateRef> > backward_graph(num_states);
    vector<vector<int> > backward_graph_costs(num_states);
    for (int i = 0; i < transitions_by_op.size(); i++) {
        const vector<AbstractTransition> &transitions = transitions_by_op[i];
        for (int j = 0; j < transitions.size(); j++) {
            const AbstractTransition &trans = transitions[j];
            backward_graph[trans.target].push_back(trans.src);
            backward_graph_costs[trans.target].push_back(trans.cost);
        }
    }

    deque<AbstractStateRef> queue;
    for (AbstractStateRef state = 0; state < num_states; state++) {
        if (goal_states[state] == true) {
            goal_distances[state] = 0;
            queue.push_back(state);
        } else {
            goal_distances[state] = QUITE_A_LOT;
        }
    }
    breadth_first_search(backward_graph, backward_graph_costs, queue,
                         goal_distances);
}

void AtomicAbstraction::apply_abstraction_to_lookup_table(const vector<
                                                              AbstractStateRef> &abstraction_mapping) {
    for (int i = 0; i < lookup_table.size(); i++) {
        AbstractStateRef old_state = lookup_table[i];
        if (old_state != -1)
            lookup_table[i] = abstraction_mapping[old_state];
    }
}

void CompositeAbstraction::apply_abstraction_to_lookup_table(const vector<
                                                                 AbstractStateRef> &abstraction_mapping) {
    for (int i = 0; i < components[0]->size(); i++) {
        for (int j = 0; j < components[1]->size(); j++) {
            AbstractStateRef old_state = lookup_table[i][j];
            if (old_state != -1)
                lookup_table[i][j] = abstraction_mapping[old_state];
        }
    }
}
void Abstraction::normalize(bool simplify_labels) {
    /* Apply label simplification and remove duplicate transitions.

     This is called right before an abstraction is merged with
     another through a product operation.

     Note that we could also normalize between merging and shrinking
     (e.g. to make the distance computations cheaper) but don't,
     because the costs are much higher than the benefits in our
     experiments.
     */
    // dump();

    OperatorRegistry *op_reg = 0;
    if (simplify_labels) {
        op_reg = new OperatorRegistry(relevant_operators, varset);
        op_reg->statistics();
    }

    typedef vector<pair<AbstractStateRef, int> > StateBucket;

    /* First, partition by target state. Also replace operators by
     canonical operators and clear away the transitions that have
     been processed. */
    vector<StateBucket> target_buckets(num_states);
    for (int op_no = 0; op_no < transitions_by_op.size(); op_no++) {
        vector<AbstractTransition> &transitions = transitions_by_op[op_no];
        if (!transitions.empty()) {
            int canon_op_no;
            if (op_reg) {
                const Operator *op = &g_operators[op_no];
                const Operator *canon_op = op_reg->get_canonical_operator(op);
                canon_op_no = get_op_index(canon_op);
            } else {
                canon_op_no = op_no;
            }
            for (int i = 0; i < transitions.size(); i++) {
                const AbstractTransition &t = transitions[i];
                target_buckets[t.target].push_back(
                    make_pair(t.src, canon_op_no));
            }
            vector<AbstractTransition> ().swap(transitions);
        }
    }

    // Second, partition by src state.
    vector<StateBucket> src_buckets(num_states);

    for (AbstractStateRef target = 0; target < num_states; target++) {
        StateBucket &bucket = target_buckets[target];
        for (int i = 0; i < bucket.size(); i++) {
            AbstractStateRef src = bucket[i].first;
            int op_no = bucket[i].second;
            src_buckets[src].push_back(make_pair(target, op_no));
        }
    }
    vector<StateBucket> ().swap(target_buckets);

    // Finally, partition by operator and drop duplicates.
    for (AbstractStateRef src = 0; src < num_states; src++) {
        StateBucket &bucket = src_buckets[src];
        for (int i = 0; i < bucket.size(); i++) {
            int target = bucket[i].first;
            int op_no = bucket[i].second;
            int op_cost = g_operators[op_no].get_cost();             //This works w/o reference to op_reg because we only reduce lables with the same cost.
            vector<AbstractTransition> &op_bucket = transitions_by_op[op_no];
            AbstractTransition trans(src, target, op_cost);
            if (op_bucket.empty() || op_bucket.back() != trans)
                op_bucket.push_back(trans);
        }
    }

    delete op_reg;
    // dump();
    //	dump_transitions_by_src();
}

void Abstraction::build_atomic_abstractions(vector<Abstraction *> &result) {
    assert(result.empty());
    cout << "Building atomic abstractions... " << flush;
    int var_count = g_variable_domain.size();

    // Step 1: Create the abstraction objects without transitions.
    for (int var_no = 0; var_no < var_count; var_no++)
        result.push_back(new AtomicAbstraction(var_no));

    // Step 2: Add transitions.
    for (int op_no = 0; op_no < g_operators.size(); op_no++) {
        const Operator *op = &g_operators[op_no];
        const vector<Prevail> &prev = op->get_prevail();
        const int op_cost = op->get_cost();
        for (int i = 0; i < prev.size(); i++) {
            int var = prev[i].var;
            int value = prev[i].prev;
            Abstraction *abs = result[var];
            AbstractTransition trans(value, value, op_cost);
            abs->transitions_by_op[op_no].push_back(trans);

            if (abs->relevant_operators.empty()
                || abs->relevant_operators.back() != op)
                abs->relevant_operators.push_back(op);
        }
        const vector<PrePost> &pre_post = op->get_pre_post();
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
                AbstractTransition trans(value, post_value, op_cost);
                abs->transitions_by_op[op_no].push_back(trans);
            }
            if (abs->relevant_operators.empty()
                || abs->relevant_operators.back() != op)
                abs->relevant_operators.push_back(op);
        }
    }

    // Step 3: Compute init and goal distances and remove unreachable parts.
    for (int var_no = 0; var_no < var_count; var_no++)
        result[var_no]->compute_distances();

    cout << "done!" << endl;
}

AtomicAbstraction::AtomicAbstraction(int variable_)
    : variable(variable_) {
    varset.push_back(variable);
    /*
     This generates the nodes of the atomic abstraction, but not the
     arcs: It is more efficient to generate all arcs of all atomic
     abstractions simultaneously.
     */
    int range = g_variable_domain[variable];

    int init_value = (*g_initial_state)[variable];
    int goal_value = -1;
    for (int goal_no = 0; goal_no < g_goal.size(); goal_no++) {
        if (g_goal[goal_no].first == variable) {
            assert(goal_value == -1);
            goal_value = g_goal[goal_no].second;
        }
    }

    num_states = range;
    lookup_table.reserve(range);
    init_distances.resize(num_states, INVALID);
    goal_distances.resize(num_states, INVALID);
    goal_states.resize(num_states, false);
    for (int value = 0; value < range; value++) {
        if (value == goal_value || goal_value == -1) {
            goal_distances[value] = 0;
            goal_states[value] = true;
        }
        if (value == init_value)
            init_state = value;
        lookup_table.push_back(value);
    }
}
CompositeAbstraction::CompositeAbstraction(Abstraction *abs1,
                                           Abstraction *abs2, bool simplify_labels,
                                           bool normalize_after_compostition) {
    assert(abs1->is_solvable() && abs2->is_solvable());

    components[0] = abs1;
    components[1] = abs2;

    ::set_union(abs1->varset.begin(), abs1->varset.end(), abs2->varset.begin(),
                abs2->varset.end(), back_inserter(varset));
    if (simplify_labels) {
        if (varset.size() != abs1->varset.size() + abs2->varset.size()) {
            cout << "error: label simplification is only correct "
                 << "for orthogonal compositions" << endl;
            ::exit(1);
        }
    }

    num_states = abs1->size() * abs2->size();
    init_distances.resize(num_states, INVALID);
    goal_distances.resize(num_states, INVALID);
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

    for (int i = 0; i < abs1->relevant_operators.size(); i++)
        abs1->relevant_operators[i]->marker1 = true;
    for (int i = 0; i < abs2->relevant_operators.size(); i++)
        abs2->relevant_operators[i]->marker2 = true;

    // HACK! Normalization should be done differently. This size() > 1
    // test is just a hack to make it work for linear abstraction
    // strategies. See issue68.
    if (abs1->varset.size() > 1)
        abs1->normalize(simplify_labels && !normalize_after_compostition);
    else if (abs2->varset.size() > 1)
        abs2->normalize(simplify_labels && !normalize_after_compostition);

    int multiplier = abs2->size();
    for (int op_no = 0; op_no < g_operators.size(); op_no++) {
        const Operator *op = &g_operators[op_no];
        bool relevant1 = op->marker1;
        bool relevant2 = op->marker2;
        if (relevant1 || relevant2) {
            vector<AbstractTransition> &transitions = transitions_by_op[op_no];
            relevant_operators.push_back(op);
            const vector<AbstractTransition> &bucket1 =
                abs1->transitions_by_op[op_no];
            const vector<AbstractTransition> &bucket2 =
                abs2->transitions_by_op[op_no];
            if (relevant1 && relevant2) {
                transitions.reserve(bucket1.size() * bucket2.size());
                for (int i = 0; i < bucket1.size(); i++) {
                    int src1 = bucket1[i].src;
                    int target1 = bucket1[i].target;
                    int cost1 = bucket1[i].cost;
                    for (int j = 0; j < bucket2.size(); j++) {
                        int src2 = bucket2[j].src;
                        int target2 = bucket2[j].target;
                        int cost2 = bucket2[j].cost;
                        int src = src1 * multiplier + src2;
                        int target = target1 * multiplier + target2;
                        int min_cost = ::min(cost1, cost2);
                        transitions.push_back(AbstractTransition(src, target,
                                                                 min_cost));
                    }
                }
            } else if (relevant1) {
                assert(!relevant2);
                transitions.reserve(bucket1.size() * abs2->size());
                for (int i = 0; i < bucket1.size(); i++) {
                    int src1 = bucket1[i].src;
                    int target1 = bucket1[i].target;
                    int cost1 = bucket1[i].cost;
                    for (int s2 = 0; s2 < abs2->size(); s2++) {
                        int src = src1 * multiplier + s2;
                        int target = target1 * multiplier + s2;
                        transitions.push_back(AbstractTransition(src, target,
                                                                 cost1));
                    }
                }
            } else if (relevant2) {
                assert(!relevant1);
                transitions.reserve(bucket2.size() * abs1->size());
                for (int i = 0; i < bucket2.size(); i++) {
                    int src2 = bucket2[i].src;
                    int target2 = bucket2[i].target;
                    int cost2 = bucket2[i].cost;
                    for (int s1 = 0; s1 < abs1->size(); s1++) {
                        int src = s1 * multiplier + src2;
                        int target = s1 * multiplier + target2;
                        transitions.push_back(AbstractTransition(src, target,
                                                                 cost2));
                    }
                }
            }
        }
    }

    for (int i = 0; i < abs1->relevant_operators.size(); i++)
        abs1->relevant_operators[i]->marker1 = false;
    for (int i = 0; i < abs2->relevant_operators.size(); i++)
        abs2->relevant_operators[i]->marker2 = false;

    // dump();
    compute_distances();
    // dump();

    //TODO - added this normalization (right before next shrink step).
    //anyway, normalization should be done from here and not in the shrink method of abstraction.c.
    if (normalize_after_compostition)
        normalize(true);
}

AbstractStateRef AtomicAbstraction::get_abstract_state(const State &state) const {
    int value = state[variable];
    return lookup_table[value];
}

AbstractStateRef CompositeAbstraction::get_abstract_state(const State &state) const {
    AbstractStateRef state1 = components[0]->get_abstract_state(state);
    AbstractStateRef state2 = components[1]->get_abstract_state(state);
    if (state1 == -1 || state2 == -1)
        return -1;
    return lookup_table[state1][state2];
}

void Abstraction::partition_into_buckets(
    vector<vector<AbstractStateRef> > &buckets,
    ShrinkStrategy shrink_strategy) const {
    assert(buckets.empty());

    typedef vector<AbstractStateRef> Bucket;

    // Classify states by f and h value.
    vector<vector<Bucket> > states_by_f_and_h;
    states_by_f_and_h.resize(max_f + 1);
    for (int f = 0; f <= max_f; f++)
        states_by_f_and_h[f].resize(min(f, max_h) + 1);
    for (AbstractStateRef state = 0; state < num_states; state++) {
        int g = init_distances[state];
        int h = goal_distances[state];
        if (g == QUITE_A_LOT || h == QUITE_A_LOT)
            continue;

        int f = g + h;

        if (shrink_strategy == SHRINK_RANDOM) {
            // Put all into the same bucket.
            f = h = 0;
        }

        assert(f >= 0 && f < states_by_f_and_h.size());
        assert(h >= 0 && h < states_by_f_and_h[f].size());
        states_by_f_and_h[f][h].push_back(state);
    }

    if (shrink_strategy == SHRINK_HIGH_F_LOW_H || shrink_strategy
        == SHRINK_RANDOM) {
        for (int f = max_f; f >= 0; f--) {
            for (int h = 0; h < states_by_f_and_h[f].size(); h++) {
                Bucket &bucket = states_by_f_and_h[f][h];
                if (!bucket.empty()) {
                    buckets.push_back(Bucket());
                    buckets.back().swap(bucket);
                }
            }
        }
    } else if (shrink_strategy == SHRINK_LOW_F_LOW_H) {
        for (int f = 0; f <= max_f; f++) {
            for (int h = 0; h < states_by_f_and_h[f].size(); h++) {
                Bucket &bucket = states_by_f_and_h[f][h];
                if (!bucket.empty()) {
                    buckets.push_back(Bucket());
                    buckets.back().swap(bucket);
                }
            }
        }
    } else if (shrink_strategy == SHRINK_HIGH_F_HIGH_H) {
        for (int f = max_f; f >= 0; f--) {
            for (int h = states_by_f_and_h[f].size() - 1; h >= 0; h--) {
                Bucket &bucket = states_by_f_and_h[f][h];
                if (!bucket.empty()) {
                    buckets.push_back(Bucket());
                    buckets.back().swap(bucket);
                }
            }
        }
    } else {
        cerr << "We should never end up here." << endl;
        exit(1);
    }
}

void Abstraction::compute_abstraction(
    vector<vector<AbstractStateRef> > &buckets, int target_size, vector<
        slist<AbstractStateRef> > &collapsed_groups) const {
    typedef slist<AbstractStateRef> Group;
    bool show_combine_buckets_warning = false;

    assert(collapsed_groups.empty());
    collapsed_groups.reserve(target_size);

    int num_states_to_go = 0;
    for (int bucket_no = 0; bucket_no < buckets.size(); bucket_no++)
        num_states_to_go += buckets[bucket_no].size();

    for (int bucket_no = 0; bucket_no < buckets.size(); bucket_no++) {
        vector<AbstractStateRef> &bucket = buckets[bucket_no];
        int remaining_state_budget = target_size - collapsed_groups.size();
        num_states_to_go -= bucket.size();
        int bucket_budget = remaining_state_budget - num_states_to_go;

        if (bucket_budget >= (int)bucket.size()) {
            // Each state in bucket can become a singleton group.
            // cout << "SHRINK NONE: " << bucket_no << endl;
            for (int i = 0; i < bucket.size(); i++) {
                Group group;
                group.push_front(bucket[i]);
                collapsed_groups.push_back(group);
            }
        } else if (bucket_budget <= 1) {
            // The whole bucket must form one group.
            // cout << "SHRINK ALL: " << bucket_no << endl;
            int remaining_buckets = buckets.size() - bucket_no;
            if (remaining_state_budget >= remaining_buckets) {
                collapsed_groups.push_back(Group());
            } else {
                if (bucket_no == 0)
                    collapsed_groups.push_back(Group());
                if (show_combine_buckets_warning) {
                    show_combine_buckets_warning = false;
                    cout
                    << "ARGH! Very small node limit, must combine buckets."
                    << endl;
                }
            }
            Group &group = collapsed_groups.back();
            group.insert(group.begin(), bucket.begin(), bucket.end());
        } else {
            // Complicated case: must combine until bucket_budget.
            // cout << "SHRINK SOME: " << bucket_no << endl;
            // First create singleton groups.
            vector<Group> groups;
            groups.resize(bucket.size());
            for (int i = 0; i < bucket.size(); i++)
                groups[i].push_front(bucket[i]);
            // Then combine groups until required size is reached.
            assert(bucket_budget >= 2 && bucket_budget < groups.size());
            while (groups.size() > bucket_budget) {
                int pos1 = ((unsigned int)rand()) % groups.size();
                int pos2;
                do {
                    pos2 = ((unsigned int)rand()) % groups.size();
                } while (pos1 == pos2);
                groups[pos1].splice(groups[pos1].begin(), groups[pos2]);
                swap(groups[pos2], groups.back());
                assert(groups.back().empty());
                groups.pop_back();
            }
            // Finally add these groups to the result.
            for (int i = 0; i < groups.size(); i++) {
                collapsed_groups.push_back(Group());
                collapsed_groups.back().swap(groups[i]);
            }
        }
    }
}

typedef vector<pair<int, int> > SuccessorSignature;

struct Signature {
    int h;
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h_, int group_, const SuccessorSignature &succ_signature_,
              int state_)
        : h(h_), group(group_), succ_signature(succ_signature_), state(state_) {
    }

    bool matches(const Signature &other) const {
        return h == other.h && group == other.group && succ_signature
               == other.succ_signature;
    }

    bool operator<(const Signature &other) const {
        if (h != other.h)
            return h < other.h;
        if (group != other.group)
            return group < other.group;
        if (succ_signature != other.succ_signature)
            return succ_signature < other.succ_signature;
        return state < other.state;
    }
};

//Given two successive signatures and the pairs of labels that should be reduced, returns true if the source states are bisimilar (after reducing the labels)
bool Abstraction::are_bisimilar_wrt_label_reduction(
    const vector<pair<int, int> > &succ_sig1,
    const vector<pair<int, int> > &succ_sig2,
    const vector<pair<int, int> > &pairs_of_labels_to_reduce) const {
    if (succ_sig1 != succ_sig2) {
        for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
            if (::find(succ_sig2.begin(), succ_sig2.end(), succ_sig1[trans1])
                == succ_sig2.end()) {
                int op_no = succ_sig1[trans1].first;
                int target = succ_sig1[trans1].second;
                bool found_reduced_trans = false;
                for (int pair = 0; pair < pairs_of_labels_to_reduce.size(); pair++) {
                    ::pair<int, int> pair_to_find = make_pair(-1, -1);
                    if (pairs_of_labels_to_reduce[pair].first == op_no)
                        pair_to_find.first =
                            pairs_of_labels_to_reduce[pair].second;
                    else if (pairs_of_labels_to_reduce[pair].second == op_no)
                        pair_to_find.first =
                            pairs_of_labels_to_reduce[pair].first;
                    if (pair_to_find.first != -1) {
                        pair_to_find.second = target;
                        if (::find(succ_sig2.begin(), succ_sig2.end(),
                                   pair_to_find) != succ_sig2.end()) {
                            found_reduced_trans = true;
                            break;
                        }
                    }
                }
                if (!found_reduced_trans)
                    return false;
            }
        }

        for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
            if (::find(succ_sig1.begin(), succ_sig1.end(), succ_sig2[trans2])
                == succ_sig1.end()) {
                int op_no = succ_sig2[trans2].first;
                int target = succ_sig2[trans2].second;
                bool found_reduced_trans = false;
                for (int pair = 0; pair < pairs_of_labels_to_reduce.size(); pair++) {
                    ::pair<int, int> pair_to_find = make_pair(-1, -1);
                    if (pairs_of_labels_to_reduce[pair].first == op_no)
                        pair_to_find.first =
                            pairs_of_labels_to_reduce[pair].second;
                    else if (pairs_of_labels_to_reduce[pair].second == op_no)
                        pair_to_find.first =
                            pairs_of_labels_to_reduce[pair].first;
                    if (pair_to_find.first != -1) {
                        pair_to_find.second = target;
                        if (::find(succ_sig1.begin(), succ_sig1.end(),
                                   pair_to_find) != succ_sig1.end()) {
                            found_reduced_trans = true;
                            break;
                        }
                    }
                }
                if (!found_reduced_trans)
                    return false;
            }
        }
    }
    return true;
}

bool Abstraction::are_bisimilar(const vector<pair<int, int> > &succ_sig1,
                                const vector<pair<int, int> > &succ_sig2, bool ignore_all_labels,
                                bool greedy_bisim, bool further_label_reduction,
                                const vector<int> &group_to_h, int source_h_1, int source_h_2,
                                const vector<pair<int, int> > &pairs_of_labels_to_reduce) const {
    if (further_label_reduction)
        return are_bisimilar_wrt_label_reduction(succ_sig1, succ_sig2,
                                                 pairs_of_labels_to_reduce);
    if (succ_sig1 != succ_sig2) {
        for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
            bool relevant = !greedy_bisim
                            || group_to_h[succ_sig1[trans1].second] < source_h_1;
            if (relevant && ::find(succ_sig2.begin(), succ_sig2.end(),
                                   succ_sig1[trans1]) == succ_sig2.end()) {
                bool found_equivalent_trans = false;
                for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
                    if ((ignore_all_labels && succ_sig1[trans1].second
                         == succ_sig2[trans2].second) || (!ignore_all_labels
                                                          && succ_sig1[trans1] == succ_sig2[trans2])) {
                        found_equivalent_trans = true;
                        break;
                    }
                }
                if (!found_equivalent_trans)
                    return false;
            }
        }
        for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
            bool relevant = !greedy_bisim
                            || group_to_h[succ_sig2[trans2].second] < source_h_2;
            if (relevant && ::find(succ_sig1.begin(), succ_sig1.end(),
                                   succ_sig2[trans2]) == succ_sig1.end()) {
                bool found_equivalent_trans = false;
                for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
                    if ((ignore_all_labels && succ_sig1[trans1].second
                         == succ_sig2[trans2].second) || (!ignore_all_labels
                                                          && succ_sig1[trans1] == succ_sig2[trans2])) {
                        found_equivalent_trans = true;
                        break;
                    }
                }
                if (!found_equivalent_trans)
                    return false;
            }
        }
    }
    return true;
}

//Given two successive signatures of two different states, this method adds to pairs,
//all pairs of labels l1,l2, (applicable from s1 and s2 respectively) that have the same target group t,
//but either (s1,l1,t') or (s2,l2,t') for t' != t.
//TODO - this is where there is some work to be done on choosing the labels pairs to reduce!
void Abstraction::add_relevant_reducible_op_pairs(
    const vector<pair<int, int> > &succ_sig1,
    const vector<pair<int, int> > &succ_sig2,
    vector<pair<int, int> > &pairs) const {
    int num_of_pairs_added = 0;

    for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
        if (::find(succ_sig2.begin(), succ_sig2.end(), succ_sig1[trans1])
            == succ_sig2.end()) {
            for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
                if (succ_sig1[trans1].second == succ_sig2[trans2].second
                    && g_operators[succ_sig1[trans1].first].affect_a_common_variable(
                        &(g_operators[succ_sig2[trans2].first]))
                    && g_operators[succ_sig1[trans1].first].have_same_op_name(
                        &(g_operators[succ_sig2[trans2].first]))) {
                    //						&& g_operators[succ_sig1[trans1].first].get_distance_from_op(&(g_operators[succ_sig2[trans2].first])) < 5) {
                    pairs.push_back(make_pair(::min(succ_sig1[trans1].first,
                                                    succ_sig2[trans2].first), ::max(
                                                  succ_sig1[trans1].first, succ_sig2[trans2].first)));
                    num_of_pairs_added++;
                    break;                     //TODO - removed this
                }
            }
        }
    }

    for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
        if (::find(succ_sig1.begin(), succ_sig1.end(), succ_sig2[trans2])
            == succ_sig1.end()) {
            for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
                if (succ_sig2[trans2].second == succ_sig1[trans1].second
                    && g_operators[succ_sig1[trans1].first].affect_a_common_variable(
                        &(g_operators[succ_sig2[trans2].first]))
                    && g_operators[succ_sig1[trans1].first].have_same_op_name(
                        &(g_operators[succ_sig2[trans2].first]))) {
                    //						&& g_operators[succ_sig1[trans1].first].get_distance_from_op(&(g_operators[succ_sig2[trans2].first])) < 5) {
                    pairs.push_back(make_pair(::min(succ_sig2[trans2].first,
                                                    succ_sig1[trans1].first), ::max(
                                                  succ_sig2[trans2].first, succ_sig1[trans1].first)));
                    num_of_pairs_added++;
                    break;                     //TODO - removed this
                }
            }
        }
    }
    ::sort(pairs.begin(), pairs.end());
    pairs.erase(::unique(pairs.begin(), pairs.end()), pairs.end());
}
void Abstraction::compute_abstraction_dfp_action_cost_support(int target_size,
                                                              vector<slist<AbstractStateRef> > &collapsed_groups,
                                                              bool enable_greedy_bisimulation) const {
//	cout << "Max h value is: " << max_h << endl;
    //vector<bool> used_h(num_states, false);
    vector<int> h_to_h_group(max_h + 1, -1);
    int num_of_used_h = 0;

    for (int state = 0; state < num_states; state++) {
        int h = goal_distances[state];
        if (h != QUITE_A_LOT && init_distances[state] != QUITE_A_LOT) {
            if (h_to_h_group[h] == -1) {
                h_to_h_group[h] = num_of_used_h;
                num_of_used_h++;
            }
        }
    }
//	cout << "Number of used h's: " << num_of_used_h << endl;

    vector<int> state_to_group(num_states);     //vector containing the states' group number.
    vector<int> group_to_h(num_states, -1);     //vector containing the groups' h-value.
    for (int state = 0; state < num_states; state++) {
        int h = goal_distances[state];
        if (h == QUITE_A_LOT || init_distances[state] == QUITE_A_LOT) {
            state_to_group[state] = -1;
        } else {
            assert(h >= 0 && h <= max_h);
            int group = h_to_h_group[h];
            state_to_group[state] = group;
            group_to_h[group] = group_to_h[group] == -1 ? h : ::min(h, group_to_h[group]);
        }
    }
    int num_groups = num_of_used_h;
    vector<bool> h_group_done(num_of_used_h, false);
    vector<Signature> signatures;
    signatures.reserve(num_states + 2);
    bool done = false;

    while (!done) {
        done = true;
        // Compute state signatures.
        // Add sentinels to the start and end.
        signatures.clear();
        signatures.push_back(Signature(-1, -1, SuccessorSignature(), -1));
        for (int state = 0; state < num_states; state++) {
            int h = goal_distances[state];
            if (h == QUITE_A_LOT || init_distances[state] == QUITE_A_LOT) {
                h = -1;
                assert(state_to_group[state] == -1);
                Signature signature(h, state_to_group[state], SuccessorSignature(),
                                    state);
            }
            Signature signature(h, state_to_group[state], SuccessorSignature(),
                                state);
            signatures.push_back(signature);
        }
        signatures.push_back(Signature(max_h + 1, -1, SuccessorSignature(), -1));
        for (int op_no = 0; op_no < transitions_by_op.size(); op_no++) {
            const vector<AbstractTransition> &transitions =
                transitions_by_op[op_no];
            for (int i = 0; i < transitions.size(); i++) {
                const AbstractTransition &trans = transitions[i];
                int src_group = state_to_group[trans.src];
                int target_group = state_to_group[trans.target];
                if (src_group != -1 && target_group != -1) {
                    assert(signatures[trans.src + 1].state == trans.src);
                    signatures[trans.src + 1].succ_signature.push_back(
                        make_pair(op_no, target_group));
                }
            }
        }
        for (int i = 0; i < signatures.size(); i++) {
            SuccessorSignature &succ_sig = signatures[i].succ_signature;
            ::sort(succ_sig.begin(), succ_sig.end());
            succ_sig.erase(::unique(succ_sig.begin(), succ_sig.end()),
                           succ_sig.end());
        }

        assert(signatures.size() == num_states + 2);
        ::sort(signatures.begin(), signatures.end());
        // TODO: More efficient to sort an index set than to shuffle
        //       the whole signatures around?

        int sig_start = 0;
        while (true) {
            int h = signatures[sig_start].h;
            if (h > max_h) {
                // We have hit the end sentinel.
                assert(h == max_h + 1);
                assert(sig_start + 1 == signatures.size());
                break;
            }
            assert(h >= -1);
            assert(h <= max_h);

            if (h == -1 || h_group_done[h_to_h_group[h]]) {
                while (signatures[sig_start].h == h)
                    sig_start++;
                continue;
            }


            int num_old_groups = 0;
            int num_new_groups = 0;
            int num_new_groups_greedy_bisimulation = 0;
            int sig_end;
            for (sig_end = sig_start; signatures[sig_end].h == h; sig_end++) {
                // cout << "@" << sig_start << "@" << sig_end << flush;

                const Signature &prev_sig = signatures[sig_end - 1];
                const Signature &curr_sig = signatures[sig_end];

                /*
                 cout << "@" << prev_sig.group
                 << "@" << curr_sig.group << endl;
                 */

                if (sig_end == sig_start)
                    assert(prev_sig.group != curr_sig.group);

                if (prev_sig.group != curr_sig.group) {
                    num_old_groups++;
                    num_new_groups++;
                    num_new_groups_greedy_bisimulation++;
                } else if (prev_sig.succ_signature != curr_sig.succ_signature) {
                    num_new_groups++;
                    if (enable_greedy_bisimulation && !are_bisimilar(
                            prev_sig.succ_signature, curr_sig.succ_signature,
                            false, enable_greedy_bisimulation, false, group_to_h, h, curr_sig.h, vector<pair<int,
                                                                                                             int> > ()))
                        num_new_groups_greedy_bisimulation++;
                }
            }
            assert(sig_end > sig_start);

            bool use_greedy_bisimulation = false;

            if (num_groups - num_old_groups + num_new_groups > target_size) {
                // Can't split the groups for this h value -- would exceed
                // bound on abstract state number.
                //				cout << "can't split groups for h = " << h
                //						<< ", num_new_groups = " << num_new_groups << endl;
                h_group_done[h_to_h_group[h]] = true;
                //This is where dfp gives up on bisimulation for this group
                if (enable_greedy_bisimulation && num_groups - num_old_groups
                    + num_new_groups_greedy_bisimulation <= target_size) {
                    use_greedy_bisimulation = true;
//						cout
//								<< "CAN SPLIT USING GREEDY.. num of new groups relaxed: "
//								<< num_new_groups_greedy_bisimulation << endl;
//						cout << "num of total groups will be: " << num_groups
//								- num_old_groups + num_new_groups_greedy_bisimulation
//								<< endl;
                }
            }

            if ((!use_greedy_bisimulation && !h_group_done[h_to_h_group[h]] && num_new_groups
                 != num_old_groups) || (use_greedy_bisimulation
                                        && num_new_groups_greedy_bisimulation != num_old_groups)) {
                // Split the groups.
                done = false;
                //this is needed since the checks on further reduction are not always done on the full set of reducible pairs
                bool performed_split = false;
                int new_group_no = -1;
                for (int i = sig_start; i < sig_end; i++) {
                    const Signature &prev_sig = signatures[i - 1];
                    const Signature &curr_sig = signatures[i];
                    if (prev_sig.group != curr_sig.group) {
                        // Start first group of a block; keep old group no.
                        new_group_no = curr_sig.group;
                    } else if ((!use_greedy_bisimulation && prev_sig.succ_signature
                                != curr_sig.succ_signature) || (use_greedy_bisimulation
                                                                && !are_bisimilar(prev_sig.succ_signature,
                                                                                  curr_sig.succ_signature, false,
                                                                                  use_greedy_bisimulation,
                                                                                  false, group_to_h, h, curr_sig.h,
                                                                                  vector<pair<int, int> > ()))) {
                        new_group_no = num_groups++;
                        performed_split = true;
                        assert(num_groups <= target_size);
                    }

                    assert(new_group_no != -1);
                    state_to_group[curr_sig.state] = new_group_no;
                    group_to_h[new_group_no] = h;
                }
                if (use_greedy_bisimulation && performed_split) {
                    h_group_done[h] = false;
//						cout << "New num of groups is: " << num_groups << endl;
                }
            }
            sig_start = sig_end;
        }
    }
    assert(collapsed_groups.empty());
    collapsed_groups.resize(num_groups);
    // int total_size = 0;
    for (int state = 0; state < num_states; state++) {
        int group = state_to_group[state];
        if (group != -1) {
            assert(group >= 0 && group < num_groups);
            collapsed_groups[group].push_front(state);
            // total_size++;
        }
    }
}
//void Abstraction::compute_abstraction_dfp_with_options(int target_size, vector<
//		slist<AbstractStateRef> > &collapsed_groups,
//		bool enable_greedy_bisimulation, bool enable_further_label_reduction,
//		bool prefer_greedy_over_label_reduction,
//		bool prefer_relaxation_with_more_groups) const {
//
//	cout << "dfp options - greedy = " << enable_greedy_bisimulation
//			<< ", FTL = " << enable_further_label_reduction
//			<< ", greedy before FTL = " << prefer_greedy_over_label_reduction
//			<< ", use maximal of the two = "
//			<< prefer_relaxation_with_more_groups << endl;
//
//	int num_groups = max_h + 1;
//
//	vector<int> state_to_group(num_states); //vector containing the states' group number.
//	vector<int> group_to_h(num_states, -1); //vector containing the groups' h-value.
//
//	vector<pair<int, int> > pairs_of_labels_to_reduce;
//	if (enable_further_label_reduction)
//		pairs_of_labels_to_reduce.reserve(g_operators.size()
//				* g_operators.size()); //TODO- check this size!
//
//	for (int state = 0; state < num_states; state++) {
//		int h = goal_distances[state];
//		if (h == QUITE_A_LOT || init_distances[state] == QUITE_A_LOT) {
//			state_to_group[state] = -1;
//		} else {
//			assert(h >= 0 && h <= max_h);
//			state_to_group[state] = h;
//			group_to_h[h] = group_to_h[h] == -1 ? h : ::min(h, group_to_h[h]);
//		}	if (enable_further_label_reduction)
//pairs_of_labels_to_reduce.reserve(g_operators.size()
//		* g_operators.size()); //TODO- check this size!
//	}
//
//	vector<bool> h_done(max_h + 1, false);
//	vector<Signature> signatures;
//	signatures.reserve(num_states + 2);
//	bool done = false;
//	while (!done) {
//		done = true;
//		if (enable_further_label_reduction) {
//			pairs_of_labels_to_reduce.clear();
//			//			cout << "cleared pairs of labels! " << endl;
//		}
//		// Compute state signatures.
//		// Add sentinels to the start and end.
//		signatures.clear();
//		signatures.push_back(Signature(-1, -1, SuccessorSignature(), -1));
//		for (int state = 0; state < num_states; state++) {
//			int h = goal_distances[state];
//			if (h == QUITE_A_LOT || init_distances[state] == QUITE_A_LOT) {
//				h = -1;
//				assert(state_to_group[state] == -1);
//			}
//			Signature signature(h, state_to_group[state], SuccessorSignature(),
//					state);
//			signatures.push_back(signature);
//		}
//		signatures.push_back(Signature(max_h + 1, -1, SuccessorSignature(), -1));
//		for (int op_no = 0; op_no < transitions_by_op.size(); op_no++) {
//			const vector<AbstractTransition> &transitions =
//					transitions_by_op[op_no];
//			for (int i = 0; i < transitions.size(); i++) {
//				const AbstractTransition &trans = transitions[i];
//				int src_group = state_to_group[trans.src];
//				int target_group = state_to_group[trans.target];
//				if (src_group != -1 && target_group != -1) {
//					assert(signatures[trans.src + 1].state == trans.src);
//					signatures[trans.src + 1].succ_signature.push_back(
//							make_pair(op_no, target_group));
//				}
//			}
//		}
//		for (int i = 0; i < signatures.size(); i++) {
//			SuccessorSignature &succ_sig = signatures[i].succ_signature;
//			::sort(succ_sig.begin(), succ_sig.end());
//			succ_sig.erase(::unique(succ_sig.begin(), succ_sig.end()),
//					succ_sig.end());
//		}
//
//		assert(signatures.size() == num_states + 2);
//		::sort(signatures.begin(), signatures.end());
//		// TODO: More efficient to sort an index set than to shuffle
//		//       the whole signatures around?
//
//		int sig_start = 0;
//		while (true) {
//			int h = signatures[sig_start].h;
//			if (h > max_h) {
//				// We have hit the end sentinel.
//				assert(h == max_h + 1);
//				assert(sig_start + 1 == signatures.size());
//				break;
//			}
//			assert(h >= -1);
//			assert(h <= max_h);
//			if (h == -1 || h_done[h]) {
//				while (signatures[sig_start].h == h)
//					sig_start++;
//				continue;
//			}
//
//			if (enable_further_label_reduction) {
//				pairs_of_labels_to_reduce.clear();
//			}
//
//			int num_old_groups = 0;
//			int num_new_groups = 0;
//			int num_new_groups_greedy_bisimulation = 0;
//			int num_new_groups_further_label_reduction = 0;
//			int num_new_groups_reducing_all_labels = 0;
//			int sig_end;
//			for (sig_end = sig_start; signatures[sig_end].h == h; sig_end++) {
//
//				// cout << "@" << sig_start << "@" << sig_end << flush;
//
//				const Signature &prev_sig = signatures[sig_end - 1];
//				const Signature &curr_sig = signatures[sig_end];
//
//				/*
//				 cout << "@" << prev_sig.group
//				 << "@" << curr_sig.group << endl;
//				 */
//
//				if (sig_end == sig_start)
//					assert(prev_sig.group != curr_sig.group);
//
//				if (prev_sig.group != curr_sig.group) {
//					num_old_groups++;
//					num_new_groups++;
//					num_new_groups_greedy_bisimulation++;
//					num_new_groups_further_label_reduction++;
//					num_new_groups_reducing_all_labels++;
//				} else if (prev_sig.succ_signature != curr_sig.succ_signature) {
//					num_new_groups++;
//					if (enable_greedy_bisimulation && !are_bisimilar(
//							prev_sig.succ_signature, curr_sig.succ_signature,
//							false, true, false, group_to_h, h, vector<pair<int,
//									int> > ()))
//						num_new_groups_greedy_bisimulation++;
//
//					if (enable_further_label_reduction) {
//						add_relevant_reducible_op_pairs(
//								prev_sig.succ_signature,
//								curr_sig.succ_signature,
//								pairs_of_labels_to_reduce); //find operators that if reduced, would prevent the split
//						if (!are_bisimilar(prev_sig.succ_signature,
//								curr_sig.succ_signature, false, false, true,
//								group_to_h, h, pairs_of_labels_to_reduce)) {
//							num_new_groups_further_label_reduction++;
//						}
//					}
//
//				}
//			}
//			assert(sig_end > sig_start);
//
//			bool use_greedy_bisimulation = false;
//			bool use_further_label_reduction = false;
//			//			bool use_reduction_of_all_labels = false;
//			int num_of_new_groups_relaxed = -1;
//			bool using_relaxed = false;
//
//			if (num_groups - num_old_groups + num_new_groups > target_size) {
//				// Can't split the groups for this h value -- would exceed
//				// bound on abstract state number.
//				//				cout << "can't split groups for h = " << h
//				//						<< ", num_new_groups = " << num_new_groups << endl;
//				h_done[h] = true;
//				//This is where dfp gives up on bisimulation for this group
//				if (enable_greedy_bisimulation && num_groups - num_old_groups
//						+ num_new_groups_greedy_bisimulation <= target_size) {
//					use_greedy_bisimulation = true;
//					using_relaxed = true;
//					num_of_new_groups_relaxed
//							= num_new_groups_greedy_bisimulation;
//					cout
//							<< "CAN SPLIT USING GREEDY.. num of new groups relaxed: "
//							<< num_of_new_groups_relaxed << endl;
//					cout << "num of total groups will be: " << num_groups
//							- num_old_groups + num_of_new_groups_relaxed
//							<< endl;
//				}
//
//				if (enable_further_label_reduction
//						&& (num_groups - num_old_groups
//								+ num_new_groups_further_label_reduction
//								<= target_size)) {
//					use_further_label_reduction = true;
//					using_relaxed = true;
//					num_of_new_groups_relaxed
//							= num_new_groups_further_label_reduction;
//					cout
//							<< "CAN SPLIT USING FLR.. (APPROXIMATE) num of new groups relaxed: "
//							<< num_of_new_groups_relaxed << endl;
//					cout << "(APPROXIMATE) num of total groups will be: "
//							<< num_groups - num_old_groups
//									+ num_of_new_groups_relaxed << endl;
//				}
//
//				if (use_greedy_bisimulation && use_further_label_reduction) {
//					if (prefer_relaxation_with_more_groups) {
//						if (num_new_groups_greedy_bisimulation
//								>= num_new_groups_further_label_reduction) {
//							use_further_label_reduction = false;
//							num_of_new_groups_relaxed
//									= num_new_groups_greedy_bisimulation;
//							cout
//									<< "GOING TO USE GREEDY... num of new groups will be "
//									<< num_of_new_groups_relaxed << endl;
//						} else {
//							use_greedy_bisimulation = false;
//							cout
//									<< "GOING TO USE FLR... num of new groups will be (approximately)"
//									<< num_of_new_groups_relaxed << endl;
//						}
//					} else if (prefer_greedy_over_label_reduction) {
//						use_further_label_reduction = false;
//						num_of_new_groups_relaxed
//								= num_new_groups_greedy_bisimulation;
//						cout
//								<< "GOING TO USE GREEDY... num of new groups will be "
//								<< num_of_new_groups_relaxed << endl;
//					} else {
//						use_greedy_bisimulation = false;
//						cout
//								<< "GOING TO USE FLR... num of new groups will be (approximately)"
//								<< num_of_new_groups_relaxed << endl;
//					}
//				}
//			}
//
//			if ((!using_relaxed && !h_done[h] && num_new_groups
//					!= num_old_groups) || (using_relaxed
//					&& num_of_new_groups_relaxed != num_old_groups)) {
//				// Split the groups.
//				done = false;
//				//this is needed since the checks on further reduction are not always done on the full set of reducible pairs
//				bool performed_split = false;
//				int new_group_no = -1;
//				for (int i = sig_start; i < sig_end; i++) {
//					const Signature &prev_sig = signatures[i - 1];
//					const Signature &curr_sig = signatures[i];
//					if (prev_sig.group != curr_sig.group) {
//						// Start first group of a block; keep old group no.
//						new_group_no = curr_sig.group;
//					} else if ((!using_relaxed && prev_sig.succ_signature
//							!= curr_sig.succ_signature) || (using_relaxed
//							&& !are_bisimilar(prev_sig.succ_signature,
//									curr_sig.succ_signature, false,
//									use_greedy_bisimulation,
//									use_further_label_reduction, group_to_h, h,
//									pairs_of_labels_to_reduce))) {
//						new_group_no = num_groups++;
//						performed_split = true;
//						assert(num_groups <= target_size);
//					}
//
//					assert(new_group_no != -1);
//					state_to_group[curr_sig.state] = new_group_no;
//					group_to_h[new_group_no] = h;
//				}
//				if (using_relaxed && performed_split) {
//					h_done[h] = false;
//					cout << "New num of groups is: " << num_groups << endl;
//				}
//			}
//			sig_start = sig_end;
//		}
//
//	}
//	assert(collapsed_groups.empty());
//	collapsed_groups.resize(num_groups);
//	// int total_size = 0;
//	for (int state = 0; state < num_states; state++) {
//		int group = state_to_group[state];
//		if (group != -1) {
//			assert(group >= 0 && group < num_groups);
//			collapsed_groups[group].push_front(state);
//			// total_size++;
//		}
//	}
//
//}
//
//void Abstraction::compute_abstraction_bisimulation(int target_size, vector<
//		slist<AbstractStateRef> > &collapsed_groups,
//		bool enable_greedy_bisimulation, bool enable_reduction_of_all_labels,
//		bool force_label_reduction_or_greedy_bisimulation) const {
//
//	int num_groups;
//
//	vector<int> state_to_group(num_states);
//	vector<int> group_to_h(num_states, -1);
//
//	bool exists_goal_state = false;
//	bool exists_non_goal_state = false;
//	for (int state = 0; state < num_states; state++) {
//		int h = goal_distances[state];
//		if (h == QUITE_A_LOT || init_distances[state] == QUITE_A_LOT) {
//			state_to_group[state] = -1;
//
//		} else {
//			assert(h >= 0 && h <= max_h);
//			if (h == 0) {
//				state_to_group[state] = 0;
//				group_to_h[0] = 0;
//				exists_goal_state = true;
//			} else if (h > 0) {
//				state_to_group[state] = 1;
//				group_to_h[1] = group_to_h[1] == -1 ? h : ::min(h,
//						group_to_h[1]);
//				exists_non_goal_state = true;
//			}
//		}
//	}
//
//	if (exists_goal_state && exists_non_goal_state)
//		num_groups = 2;
//	else
//		num_groups = 1;
//
//	//Now all goal states are in group 0 and non-goal states are in 1. Unreachable states are in -1.
//
//	vector<bool> group_done(num_states, false);
//
//	vector<Signature> signatures;
//	signatures.reserve(num_states + 2);
//
//	bool done = false;
//	while (!done) {
//		done = true;
//
//		// Compute state signatures.
//		// Add sentinels to the start and end.
//		signatures.clear();
//		signatures.push_back(Signature(-1, -1, SuccessorSignature(), -1));
//		for (int state = 0; state < num_states; state++) {
//			int h = goal_distances[state];
//			if (h == QUITE_A_LOT || init_distances[state] == QUITE_A_LOT) {
//				h = -1;
//				assert(state_to_group[state] == -1);
//			}
//			Signature signature(h, state_to_group[state], SuccessorSignature(),
//					state);
//			signatures.push_back(signature);
//		}
//		signatures.push_back(Signature(max_h + 1, -1, SuccessorSignature(), -1));
//
//		//Adds to the succ_sig of every signature, the pair <op_no, target_group>
//		//reachable by the transition op_no on the state of the signature.
//		for (int op_no = 0; op_no < transitions_by_op.size(); op_no++) {
//			const vector<AbstractTransition> &transitions =
//					transitions_by_op[op_no];
//			for (int i = 0; i < transitions.size(); i++) {
//				const AbstractTransition &trans = transitions[i];
//				int src_group = state_to_group[trans.src];
//				int target_group = state_to_group[trans.target];
//				if (src_group != -1 && target_group != -1) {
//					assert(signatures[trans.src + 1].state == trans.src);
//					signatures[trans.src + 1].succ_signature.push_back(
//							make_pair(op_no, target_group));
//				}
//			}
//		}
//		for (int i = 0; i < signatures.size(); i++) {
//			SuccessorSignature &succ_sig = signatures[i].succ_signature;
//			::sort(succ_sig.begin(), succ_sig.end());
//			succ_sig.erase(::unique(succ_sig.begin(), succ_sig.end()),
//					succ_sig.end());
//		}
//		assert(signatures.size() == num_states + 2);
//		::sort(signatures.begin(), signatures.end());
//		// TODO: More efficient to sort an index set than to shuffle
//		//       the whole signatures around?
//
//		int sig_start = 0;
//		while (true) {
//			int h = signatures[sig_start].h;
//			int group = signatures[sig_start].group;
//			// cout << sig_start << " *** " << h << endl;
//			if (h > max_h) {
//				// We have hit the end sentinel.
//				assert(h == max_h + 1);
//				assert(sig_start + 1 == signatures.size());
//				break;
//			}
//			assert(h >= -1);
//			assert(h <= max_h);
//
//			//this code skips all groups that cannot be further split
//			//(due to memory restrictions).
//			if (h == -1 || group_done[group]) {
//				while (signatures[sig_start].group == group)
//					sig_start++;
//				continue;
//			}
//
//			//This next piece of code is for computing the number of blocks after the splitting.
//			//changed the condition in the for loop to equality of groups and not h values.
//			int num_old_groups = 0;
//			int num_new_groups = 0;
//			int num_new_groups_label_reduction_or_greedy_bisimulation = 0;
//			int sig_end;
//			for (sig_end = sig_start; signatures[sig_end].group == group; sig_end++) {
//
//				// cout << "@" << sig_start << "@" << sig_end << flush;
//
//				const Signature &prev_sig = signatures[sig_end - 1];
//				const Signature &curr_sig = signatures[sig_end];
//
//				/*
//				 cout << "@" << prev_sig.group
//				 << "@" << curr_sig.group << endl;
//				 */
//
//				if (sig_end == sig_start)
//					assert(prev_sig.group != curr_sig.group);
//
//				if (prev_sig.group != curr_sig.group) {
//					num_old_groups++;
//					num_new_groups++;
//					num_new_groups_label_reduction_or_greedy_bisimulation++;
//				} else if (prev_sig.succ_signature != curr_sig.succ_signature) {
//					num_new_groups++;
//					if ((enable_greedy_bisimulation
//							|| enable_reduction_of_all_labels
//							|| force_label_reduction_or_greedy_bisimulation)
//							&& !are_bisimilar(prev_sig.succ_signature,
//									curr_sig.succ_signature,
//									enable_reduction_of_all_labels,
//									enable_greedy_bisimulation, false,
//									group_to_h, h, vector<pair<int, int> > ()))
//						num_new_groups_label_reduction_or_greedy_bisimulation++;
//				}
//			}
//			assert(sig_end > sig_start);
//
//			bool use_label_reduction_or_greedy_bisimulation =
//					force_label_reduction_or_greedy_bisimulation;
//
//			if (num_groups - num_old_groups + num_new_groups > target_size) {
//				// Can't split the group -- would exceed
//				// bound on abstract state number.
//				group_done[group] = true;
//				if ((enable_greedy_bisimulation
//						|| enable_reduction_of_all_labels
//						|| force_label_reduction_or_greedy_bisimulation)
//						&& num_groups - num_old_groups
//								+ num_new_groups_label_reduction_or_greedy_bisimulation
//								<= target_size) {
//					use_label_reduction_or_greedy_bisimulation = true;
//				}
//
//			}
//			if ((use_label_reduction_or_greedy_bisimulation
//					&& num_new_groups_label_reduction_or_greedy_bisimulation
//							!= num_old_groups)
//					|| (!use_label_reduction_or_greedy_bisimulation
//							&& !group_done[group] && num_new_groups
//							!= num_old_groups)) {
//				// Split the group into the new groups, where if two states are equivalent
//				//in any bisimulation, they will be in the same group.
//				done = false;
//
//				bool performed_split = false;
//				int new_group_no = -1;
//				for (int i = sig_start; i < sig_end; i++) {
//					const Signature &prev_sig = signatures[i - 1];
//					const Signature &curr_sig = signatures[i];
//					if (prev_sig.group != curr_sig.group) {
//						// Start first group of a block; keep old group no.
//						new_group_no = curr_sig.group;
//					} else if ((!use_label_reduction_or_greedy_bisimulation
//							&& prev_sig.succ_signature
//									!= curr_sig.succ_signature)
//							|| (use_label_reduction_or_greedy_bisimulation
//									&& !are_bisimilar(prev_sig.succ_signature,
//											curr_sig.succ_signature,
//											enable_reduction_of_all_labels,
//											enable_greedy_bisimulation, false,
//											group_to_h, h, vector<
//													pair<int, int> > ()))) {
//						new_group_no = num_groups++;
//						performed_split = true;
//						assert(num_groups <= target_size);
//					}
//					assert(new_group_no != -1);
//					state_to_group[curr_sig.state] = new_group_no;
//					group_to_h[new_group_no] = h;
//				}
//				if (use_label_reduction_or_greedy_bisimulation
//						&& performed_split)
//					group_done[group] = false;
//			}
//
//			sig_start = sig_end;
//		}
//	}
//	assert(collapsed_groups.empty());
//	collapsed_groups.resize(num_groups);
//	// int total_size = 0;
//	for (int state = 0; state < num_states; state++) {
//		int group = state_to_group[state];
//		if (group != -1) {
//			assert(group >= 0 && group < num_groups);
//			collapsed_groups[group].push_front(state);
//			// total_size++;
//		}
//	}
//	// cout << "total_size: " << total_size << endl;
//}

void Abstraction::compute_abstraction_bisimulation_action_cost_support(
    int target_size, vector<slist<AbstractStateRef> > &collapsed_groups,
    bool enable_greedy_bisimulation) const {
    int num_groups;
    vector<int> state_to_group(num_states);
    vector<int> group_to_h(num_states, -1);

    bool exists_goal_state = false;
    bool exists_non_goal_state = false;
    for (int state = 0; state < num_states; state++) {
        int h = goal_distances[state];
        bool isGoalState = goal_states[state];
        if (h == QUITE_A_LOT || init_distances[state] == QUITE_A_LOT) {
            state_to_group[state] = -1;
        } else {
            assert(h >= 0 && h <= max_h);
            if (h == 0 && isGoalState) {
                state_to_group[state] = 0;
                group_to_h[0] = 0;
                exists_goal_state = true;
            } else if (h >= 0) {
                state_to_group[state] = 1;
                group_to_h[1] = group_to_h[1] == -1 ? h : ::min(h,
                                                                group_to_h[1]);
                exists_non_goal_state = true;
            }
        }
    }

    if (exists_goal_state && exists_non_goal_state)
        num_groups = 2;
    else
        num_groups = 1;

    //Now all goal states are in group 0 and non-goal states are in 1. Unreachable states are in -1.

    vector<bool> group_done(num_states, false);

    vector<Signature> signatures;
    signatures.reserve(num_states + 2);

    bool done = false;
    while (!done) {
        done = true;
        //TODO!!!!!!!- maybe need to update group_to_h vector with new values!!!!
        // Compute state signatures.
        // Add sentinels to the start and end.
        signatures.clear();
        signatures.push_back(Signature(-1, -1, SuccessorSignature(), -1));
        for (int state = 0; state < num_states; state++) {
            int h = goal_distances[state];
            if (h == QUITE_A_LOT || init_distances[state] == QUITE_A_LOT) {
                h = -1;
                assert(state_to_group[state] == -1);
            }
            Signature signature(h, state_to_group[state], SuccessorSignature(),
                                state);
            signatures.push_back(signature);
        }
        signatures.push_back(Signature(max_h + 1, -1, SuccessorSignature(), -1));

        //Adds to the succ_sig of every signature, the pair <op_no, target_group>
        //reachable by the transition op_no on the state of the signature.
        for (int op_no = 0; op_no < transitions_by_op.size(); op_no++) {
            const vector<AbstractTransition> &transitions =
                transitions_by_op[op_no];
            for (int i = 0; i < transitions.size(); i++) {
                const AbstractTransition &trans = transitions[i];
                int src_group = state_to_group[trans.src];
                int target_group = state_to_group[trans.target];
                if (src_group != -1 && target_group != -1) {
                    assert(signatures[trans.src + 1].state == trans.src);
                    signatures[trans.src + 1].succ_signature.push_back(
                        make_pair(op_no, target_group));
                }
            }
        }
        if (enable_greedy_bisimulation)
            for (int i = 0; i < group_to_h.size(); i++)             //TODO- this is something I added (update of the group_to_h)
                group_to_h[i] = -1;

        for (int i = 0; i < signatures.size(); i++) {
            SuccessorSignature &succ_sig = signatures[i].succ_signature;
            ::sort(succ_sig.begin(), succ_sig.end());
            succ_sig.erase(::unique(succ_sig.begin(), succ_sig.end()),
                           succ_sig.end());
            //TODO - this is something I added (update of the group_to_h)
            if (enable_greedy_bisimulation && signatures[i].group > -1)
                group_to_h[signatures[i].group] =
                    group_to_h[signatures[i].group] == -1 ? signatures[i].h
                    : min(signatures[i].h,
                          group_to_h[signatures[i].group]);
        }
        assert(signatures.size() == num_states + 2);
        ::sort(signatures.begin(), signatures.end());
        // TODO: More efficient to sort an index set than to shuffle
        //       the whole signatures around?


        int sig_start = 0;
        while (true) {
            int h = signatures[sig_start].h;
            int group = signatures[sig_start].group;
            // cout << sig_start << " *** " << h << endl;
            if (h > max_h) {
                // We have hit the end sentinel.
                assert(h == max_h + 1);
                assert(sig_start + 1 == signatures.size());
                break;
            }
            assert(h >= -1);
            assert(h <= max_h);

            //this code skips all groups that cannot be further split
            //(due to memory restrictions).
            if (h == -1 || group_done[group]) {
                while (signatures[sig_start].group == group)
                    sig_start++;
                continue;
            }

            //This next piece of code is for computing the number of blocks after the splitting.
            //changed the condition in the for loop to equality of groups and not h values.
            int num_old_groups = 0;
            int num_new_groups = 0;
            int num_new_groups_label_reduction_or_greedy_bisimulation = 0;
            int sig_end;
            for (sig_end = sig_start; signatures[sig_end].group == group; sig_end++) {
                // cout << "@" << sig_start << "@" << sig_end << flush;

                const Signature &prev_sig = signatures[sig_end - 1];
                const Signature &curr_sig = signatures[sig_end];

                /*
                 cout << "@" << prev_sig.group
                 << "@" << curr_sig.group << endl;
                 */

                if (sig_end == sig_start)
                    assert(prev_sig.group != curr_sig.group);

                if (prev_sig.group != curr_sig.group) {
                    num_old_groups++;
                    num_new_groups++;
                    num_new_groups_label_reduction_or_greedy_bisimulation++;
                } else if (prev_sig.succ_signature != curr_sig.succ_signature) {
                    num_new_groups++;
                    if (enable_greedy_bisimulation && !are_bisimilar(
                            prev_sig.succ_signature, curr_sig.succ_signature,
                            false, enable_greedy_bisimulation, false,
                            group_to_h, group_to_h[prev_sig.group],
                            group_to_h[curr_sig.group],
                            vector<pair<int, int> > ()))                            //TODO - changed h to group_to_h
                        num_new_groups_label_reduction_or_greedy_bisimulation++;
                }
            }
            assert(sig_end > sig_start);

            bool use_label_reduction_or_greedy_bisimulation =
                enable_greedy_bisimulation;

            if (num_groups - num_old_groups + num_new_groups > target_size) {
                // Can't split the group -- would exceed
                // bound on abstract state number.
                group_done[group] = true;
                if (enable_greedy_bisimulation && num_groups - num_old_groups
                    + num_new_groups_label_reduction_or_greedy_bisimulation
                    <= target_size) {
                    use_label_reduction_or_greedy_bisimulation = true;
                }
            }
            if ((use_label_reduction_or_greedy_bisimulation
                 && num_new_groups_label_reduction_or_greedy_bisimulation
                 != num_old_groups)
                || (!use_label_reduction_or_greedy_bisimulation
                    && !group_done[group] && num_new_groups
                    != num_old_groups)) {
                // Split the group into the new groups, where if two states are equivalent
                //in any bisimulation, they will be in the same group.
                done = false;

                bool performed_split = false;
                int new_group_no = -1;
                for (int i = sig_start; i < sig_end; i++) {
                    const Signature &prev_sig = signatures[i - 1];
                    const Signature &curr_sig = signatures[i];
                    if (prev_sig.group != curr_sig.group) {
                        // Start first group of a block; keep old group no.
                        new_group_no = curr_sig.group;
                    } else if ((!use_label_reduction_or_greedy_bisimulation
                                && prev_sig.succ_signature
                                != curr_sig.succ_signature)
                               || (use_label_reduction_or_greedy_bisimulation
                                   && !are_bisimilar(prev_sig.succ_signature,
                                                     curr_sig.succ_signature, false,
                                                     enable_greedy_bisimulation, false,
                                                     group_to_h,
                                                     group_to_h[prev_sig.group],
                                                     group_to_h[curr_sig.group], vector<
                                                         pair<int, int> > ()))) {                                                //TODO - changed h to group_to_h
                        new_group_no = num_groups++;
                        performed_split = true;
                        assert(num_groups <= target_size);
                    }
                    assert(new_group_no != -1);
                    state_to_group[curr_sig.state] = new_group_no;
                    group_to_h[new_group_no] = curr_sig.h;                     //TODO - changed from just h.
                }
                if (use_label_reduction_or_greedy_bisimulation
                    && performed_split)
                    group_done[group] = false;
            }

            sig_start = sig_end;
        }
    }
    assert(collapsed_groups.empty());
    collapsed_groups.resize(num_groups);
    // int total_size = 0;
    for (int state = 0; state < num_states; state++) {
        int group = state_to_group[state];
        if (group != -1) {
            assert(group >= 0 && group < num_groups);
            collapsed_groups[group].push_front(state);
            // total_size++;
        }
    }
    int zero_sized_groups = 0;
    int geq_one_sized_groups = 0;
    for (int i = 0; i < collapsed_groups.size(); i++) {
        if (collapsed_groups[i].size() == 0)
            zero_sized_groups++;
        if (collapsed_groups[i].size() > 1)
            geq_one_sized_groups++;
    }
//	if (zero_sized_groups > 0)
//		cout << "NUM OF 0 SIZE GROUPS: " << zero_sized_groups << endl;
//	if (geq_one_sized_groups > 0)
//		cout << "NUM OF GEQ 1 GROUPS: " << geq_one_sized_groups << endl;
}
void Abstraction::apply_abstraction(
    vector<slist<AbstractStateRef> > &collapsed_groups) {
    typedef slist<AbstractStateRef> Group;

    vector<int> abstraction_mapping(num_states, -1);

    cout << "Size of collapsed groups =" << collapsed_groups.size() << endl;

    for (int group_no = 0; group_no < collapsed_groups.size(); group_no++) {
        Group &group = collapsed_groups[group_no];
        for (Group::iterator pos = group.begin(); pos != group.end(); ++pos) {
            AbstractStateRef state = *pos;
            assert(abstraction_mapping[state] == -1);
            abstraction_mapping[state] = group_no;
        }
    }

    int new_num_states = collapsed_groups.size();
    vector<int> new_init_distances(new_num_states, INVALID);
    vector<int> new_goal_distances(new_num_states, INVALID);
    vector<bool> new_goal_states(new_num_states, false);

    bool must_recompute_distances = false;
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
                must_recompute_distances = true;
                new_init_dist = init_distances[*pos];
            }
            if (goal_distances[*pos] < new_goal_dist) {
                must_recompute_distances = true;
                new_goal_dist = goal_distances[*pos];
            }
            new_goal_states[new_state] = new_goal_states[new_state]
                                         || goal_states[*pos]; //TODO - this forces all groups containing a goal state, to be a goal state in the new abstraction.
        }
    }

    vector<int> ().swap(init_distances);     // Release memory.
    vector<int> ().swap(goal_distances);     // Release memory.
    vector<bool> ().swap(goal_states);     // Release memory.

    vector<vector<AbstractTransition> > new_transitions_by_op(
        transitions_by_op.size());
    for (int op_no = 0; op_no < transitions_by_op.size(); op_no++) {
        const vector<AbstractTransition> &transitions =
            transitions_by_op[op_no];
        vector<AbstractTransition> &new_transitions =
            new_transitions_by_op[op_no];
        new_transitions.reserve(transitions.size());
        for (int i = 0; i < transitions.size(); i++) {
            const AbstractTransition &trans = transitions[i];
            int src = abstraction_mapping[trans.src];
            int target = abstraction_mapping[trans.target];
            int cost = trans.cost;
            if (src != -1 && target != -1)
                new_transitions.push_back(AbstractTransition(src, target, cost));
        }
    }
    vector<vector<AbstractTransition> > ().swap(transitions_by_op);

    num_states = new_num_states;
    transitions_by_op.swap(new_transitions_by_op);
    init_distances.swap(new_init_distances);
    goal_distances.swap(new_goal_distances);
    goal_states.swap(new_goal_states);
    init_state = abstraction_mapping[init_state];
    if (init_state == -1)
        cout << "init_state irrelevant; problem unsolvable" << endl;

    apply_abstraction_to_lookup_table(abstraction_mapping);

    if (init_state != -1 && must_recompute_distances) {
        cout << "Simplification was not f-preserving -- "
             << "must recompute distances." << endl;
        compute_distances();
    }
}
//TODO - consider performing normalize before actual shrink, on all bisim strategies...
//At least in gripper it improves performance...
//In further label reductions, this should at least be done, and if normalize includes
//the update of the transition by source database, this should be done so the transition
//by source database is updated.
void Abstraction::shrink(int threshold, ShrinkStrategy shrink_strategy,
                         bool force) {
    /* Shrink the abstraction to size threshold.

     This also prunes all irrelevant and unreachable states, which
     may cause the resulting size to be lower than threshold.

     Does nothing if threshold >= size() unless force is true (in
     which case it only prunes irrelevant and unreachable states).
     */
    assert(threshold >= 1);
    assert(is_solvable());

    if (size() > threshold)
        cout << "shrink by " << (size() - threshold) << " nodes" << " (from "
             << size() << " to " << threshold << ")" << endl;
    else if (force)
        cout << "shrink forced: prune unreachable/irrelevant states" << endl;
    else if (shrink_strategy >= SHRINK_DFP && shrink_strategy
             <= MAX_SHRINK_STRATEGY)
        cout << "shrink due to bisimulation strategy" << endl;
    else
        return;

    vector<slist<AbstractStateRef> > collapsed_groups;
    if (shrink_strategy == SHRINK_DFP) {
        compute_abstraction_dfp_action_cost_support(threshold,
                                                    collapsed_groups, false);
    } else if (shrink_strategy == SHRINK_DFP_ENABLE_GREEDY_BISIMULATION) {
        compute_abstraction_dfp_action_cost_support(threshold,
                                                    collapsed_groups, true);
    } else if (shrink_strategy == SHRINK_DFP_ENABLE_FURTHER_LABEL_REDUCTION) {
        //		compute_abstraction_dfp_with_options(threshold, collapsed_groups,
        //				false, true, false, false);
    } else if (shrink_strategy == SHRINK_DFP_ENABLE_GREEDY_THEN_LABEL_REDUCTION) {
        //		compute_abstraction_dfp_with_options(threshold, collapsed_groups, true,
        //				true, true, false);
    } else if (shrink_strategy == SHRINK_DFP_ENABLE_LABEL_REDUCTION_THEN_GREEDY) {
        //		compute_abstraction_dfp_with_options(threshold, collapsed_groups, true,
        //				true, false, false);
    } else if (shrink_strategy
               == SHRINK_DFP_ENABLE_LABEL_REDUCTION_AND_GREEDY_CHOOSE_MAX) {
        //		compute_abstraction_dfp_with_options(threshold, collapsed_groups, true,
        //				true, false, true);
    } else if (shrink_strategy == SHRINK_BISIMULATION) {
        //		compute_abstraction_bisimulation(threshold, collapsed_groups, false,
        //				false, false);
    } else if (shrink_strategy == SHRINK_BISIMULATION_NO_MEMORY_LIMIT) {
        compute_abstraction_bisimulation_action_cost_support(QUITE_A_LOT,
                                                             collapsed_groups, false);
        //TODO - changed this
        //		compute_abstraction_bisimulation(QUITE_A_LOT, collapsed_groups, false,
        //				false, false);
    } else if (shrink_strategy == SHRINK_GREEDY_BISIMULATION_NO_MEMORY_LIMIT) {
        compute_abstraction_bisimulation_action_cost_support(QUITE_A_LOT,
                                                             collapsed_groups, true);
        //TODO		compute_abstraction_bisimulation(QUITE_A_LOT, collapsed_groups, true,
        //				false, true);
    } else if (shrink_strategy
               == SHRINK_BISIMULATION_REDUCING_ALL_LABELS_NO_MEMORY_LIMIT) {
        //		compute_abstraction_bisimulation(QUITE_A_LOT, collapsed_groups, false,
        //				true, true);
    } else if (shrink_strategy
               == SHRINK_GREEDY_BISIMULATION_REDUCING_ALL_LABELS_NO_MEMORY_LIMIT) {
        //		compute_abstraction_bisimulation(QUITE_A_LOT, collapsed_groups, true,
        //				true, true);
    } else {
        vector<vector<AbstractStateRef> > buckets;
        partition_into_buckets(buckets, shrink_strategy);
        compute_abstraction(buckets, threshold, collapsed_groups);
        assert(collapsed_groups.size() <= threshold);
    }
    apply_abstraction(collapsed_groups);
    cout << "size of abstraction after shrink: " << size() << ", Threshold: "
         << threshold << endl;
    assert(size() <= threshold || threshold == 1);
}

bool Abstraction::is_solvable() const {
    return init_state != -1;
}

int Abstraction::get_cost(const State &state) const {
    int abs_state = get_abstract_state(state);
    if (abs_state == -1)
        return -1;
    int cost = goal_distances[abs_state];
    assert(cost != INVALID && cost != QUITE_A_LOT);
    return cost;
}

int Abstraction::memory_estimate() const {
    int result = sizeof(Abstraction);
    result += sizeof(Operator *) * relevant_operators.capacity();
    result += sizeof(vector<AbstractTransition> )
              * transitions_by_op.capacity();
    for (int i = 0; i < transitions_by_op.size(); i++)
        result += sizeof(AbstractTransition) * transitions_by_op[i].capacity();
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
    vector<const Operator *> ().swap(relevant_operators);
    vector<vector<AbstractTransition> > ().swap(transitions_by_op);
}

int Abstraction::total_transitions() const {
    int total = 0;
    for (int i = 0; i < transitions_by_op.size(); i++)
        total += transitions_by_op[i].size();
    return total;
}

int Abstraction::unique_unlabeled_transitions() const {
    vector<AbstractTransition> unique_transitions;
    for (int i = 0; i < transitions_by_op.size(); i++) {
        const vector<AbstractTransition> &trans = transitions_by_op[i];
        unique_transitions.insert(unique_transitions.end(), trans.begin(),
                                  trans.end());
    }
    ::sort(unique_transitions.begin(), unique_transitions.end());
    return unique(unique_transitions.begin(), unique_transitions.end())
           - unique_transitions.begin();
}

int Abstraction::unique_unlabeled_transitions(const vector<int> &relevant_ops) const {
    vector<AbstractTransition> unique_transitions;
    for (int i = 0; i < relevant_ops.size(); i++) {
        const vector<AbstractTransition> &trans =
            transitions_by_op[relevant_ops[i]];
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
    cout << "abstraction: " << varset.size() << "/" << g_variable_domain.size()
         << " vars, " << size() << " nodes, ";
    if (include_expensive_statistics)
        cout << unique_unlabeled_transitions();
    else
        cout << "???";
    cout << "/" << total_transitions() << " arcs, " << memory << " bytes"
         << endl << "             ";
    if (is_solvable()) {
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
    // TODO: dump relevant_operators, init_distances, goal_distances,
    //       max_f, max_g, max_h, varset?
    cout << "digraph abstract_transition_graph";
    for (int i = 0; i < varset.size(); i++)
        cout << "_" << varset[i];
    cout << " {" << endl;
    cout << "    node [shape = none] start;" << endl;
    assert(goal_distances.size() == num_states);
    for (int i = 0; i < num_states; i++) {
        bool is_init = (i == init_state);
        //bool is_goal = (goal_distances[i] == 0);
        bool is_goal = (goal_states[i] == true);
        cout << "    node [shape = " << (is_goal ? "doublecircle" : "circle")
             << "] node" << i << ";" << endl;
        if (is_init)
            cout << "    start -> node" << i << ";" << endl;
    }
    assert(transitions_by_op.size() == g_operators.size());
    for (int op_no = 0; op_no < g_operators.size(); op_no++) {
        const vector<AbstractTransition> &trans = transitions_by_op[op_no];
        for (int i = 0; i < trans.size(); i++) {
            int src = trans[i].src;
            int target = trans[i].target;
            cout << "    node" << src << " -> node" << target << " [label = o_"
                 << op_no << "];" << endl;
        }
    }
    cout << "}" << endl;
}
void Abstraction::dump_transitions_by_src() const {
    cout << "--------------------------------------------------" << endl;
    cout << "Transitions by source:" << endl;
    for (int src = 0; src < transitions_by_source.size(); src++) {
        for (int target_op = 0; target_op < transitions_by_source[src].size(); target_op++)
            cout << src << " -> "
                 << transitions_by_source[src][target_op].target
                 << " via op no. "
                 << transitions_by_source[src][target_op].op << endl;
    }
    cout << "--------------------------------------------------" << endl;
}
