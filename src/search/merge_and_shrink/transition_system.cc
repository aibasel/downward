#include "transition_system.h"

#include "labels.h"
#include "shrink_fh.h"

#include "../equivalence_relation.h"
#include "../global_operator.h"
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
 operator, and it also allows applying transition system mappings very
 efficiently.

 We rarely need to be able to efficiently query the successors of a
 given state; actually, only the distance computation requires that,
 and it simply generates such a graph representation of the
 transitions itself. Various experiments have shown that maintaining
 a graph representation permanently for the benefit of distance
 computation is not worth the overhead.
 */

const int INF = numeric_limits<int>::max();


const int TransitionSystem::PRUNED_STATE;
const int TransitionSystem::DISTANCE_UNKNOWN;


TransitionSystem::TransitionSystem(Labels *labels_)
    : labels(labels_), equivalent_labels(0), num_labels(labels->get_size()),
      transitions_by_label(g_operators.empty() ? 0 : g_operators.size() * 2 - 1),
      relevant_labels(transitions_by_label.size(), false),
      transitions_sorted_unique(true), peak_memory(0) {
    clear_distances();
}

TransitionSystem::~TransitionSystem() {
    delete equivalent_labels;
}

bool TransitionSystem::is_valid() const {
    bool valid = are_distances_computed()
            && are_transitions_sorted_unique()
            && are_equivalent_labels_computed()
            && is_label_reduced();
    if (valid) {
        assert(check_equivrel_consistent());
    }
    return valid;
}

bool TransitionSystem::check_equivrel_consistent() const {
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        int repr = label_to_representative[label_no];
        if (repr == -1 || repr != label_no) {
            if (!transitions_by_label[label_no].empty()) {
                return false;
            }
        }
    }
    for (BlockListConstIter block = equivalent_labels->begin();
         block != equivalent_labels->end(); ++block) {
        int min_label_no = *block->begin();
        if (label_to_representative[min_label_no] != min_label_no) {
            return false;
        }
        for (ElementListConstIter jt = block->begin(); jt != block->end(); ++jt) {
            if (jt == block->begin())
                continue;
            int label_no = *jt;
            if (min_label_no >= label_no) {
                return false;
            }
            if (!transitions_by_label[label_no].empty()) {
                return false;
            }
            if (label_to_representative[label_no] != min_label_no) {
                return false;
            }
        }
    }
    return true;
}

void TransitionSystem::clear_distances() {
    max_f = DISTANCE_UNKNOWN;
    max_g = DISTANCE_UNKNOWN;
    max_h = DISTANCE_UNKNOWN;
    init_distances.clear();
    goal_distances.clear();
}

void TransitionSystem::discard_states(const vector<bool> &to_be_pruned_states) {
    assert(static_cast<int>(to_be_pruned_states.size()) == num_states);
    vector<slist<AbstractStateRef> > equivalence_relation;
    equivalence_relation.reserve(num_states);
    for (int state = 0; state < num_states; ++state) {
        if (!to_be_pruned_states[state]) {
            slist<AbstractStateRef> group;
            group.push_front(state);
            equivalence_relation.push_back(group);
        }
    }
    apply_abstraction(equivalence_relation);
}

bool TransitionSystem::are_distances_computed() const {
    if (max_h == DISTANCE_UNKNOWN) {
        assert(max_f == DISTANCE_UNKNOWN);
        assert(max_g == DISTANCE_UNKNOWN);
        assert(init_distances.empty());
        assert(goal_distances.empty());
        return false;
    }
    return true;
}

void TransitionSystem::compute_distances_and_prune() {
    /* This method computes the distances of abstract states from the
       abstract initial state ("abstract g") and from the abstract
       goal states ("abstract h"). It also prunes all states that are
       unreachable (abstract g is infinite) or irrelevant (abstact h
       is infinite).

       In addition to its main job of pruning state and setting
       init_distances and goal_distances, it also sets max_f, max_g
       and max_h.
    */

    cout << tag() << flush;
    assert(!are_distances_computed());
    assert(init_distances.empty() && goal_distances.empty());

    if (init_state == PRUNED_STATE) {
        cout << "init state was pruned, no distances to compute" << endl;
        // If init_state was pruned, then everything must have been pruned.
        assert(num_states == 0);
        max_f = max_g = max_h = INF;
        return;
    }

    bool is_unit_cost = true;
    for (int label_no = 0; label_no < labels->get_size(); ++label_no) {
        if (relevant_labels[label_no]) {
            if (labels->is_current_label(label_no)) {
                if (labels->get_label_cost(label_no) != 1) {
                    is_unit_cost = false;
                    break;
                }
            }
        }
    }

    init_distances.resize(num_states, INF);
    goal_distances.resize(num_states, INF);
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
    vector<bool> to_be_pruned_states(num_states, false);
    for (int i = 0; i < num_states; ++i) {
        int g = init_distances[i];
        int h = goal_distances[i];
        // States that are both unreachable and irrelevant are counted
        // as unreachable, not irrelevant. (Doesn't really matter, of
        // course.)
        if (g == INF) {
            ++unreachable_count;
            to_be_pruned_states[i] = true;
        } else if (h == INF) {
            ++irrelevant_count;
            to_be_pruned_states[i] = true;
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
        discard_states(to_be_pruned_states);
    }
    assert(are_distances_computed());
}

static void breadth_first_search(
    const vector<vector<int> > &graph, deque<int> &queue,
    vector<int> &distances) {
    while (!queue.empty()) {
        int state = queue.front();
        queue.pop_front();
        for (size_t i = 0; i < graph[state].size(); ++i) {
            int successor = graph[state][i];
            if (distances[successor] > distances[state] + 1) {
                distances[successor] = distances[state] + 1;
                queue.push_back(successor);
            }
        }
    }
}

void TransitionSystem::compute_init_distances_unit_cost() {
    vector<vector<AbstractStateRef> > forward_graph(num_states);
    for (BlockListConstIter it = equivalent_labels->begin();
         it != equivalent_labels->end(); ++it) {
        const Block &block = *it;
        int representative_label_no = *block.begin();
        const vector<Transition> &transitions = transitions_by_label[representative_label_no];
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &trans = transitions[j];
            forward_graph[trans.src].push_back(trans.target);
        }
    }

    deque<AbstractStateRef> queue;
    for (AbstractStateRef state = 0; state < num_states; ++state) {
        if (state == init_state) {
            init_distances[state] = 0;
            queue.push_back(state);
        }
    }
    breadth_first_search(forward_graph, queue, init_distances);
}

void TransitionSystem::compute_goal_distances_unit_cost() {
    vector<vector<AbstractStateRef> > backward_graph(num_states);
    for (BlockListConstIter it = equivalent_labels->begin();
         it != equivalent_labels->end(); ++it) {
        const Block &block = *it;
        int representative_label_no = *block.begin();
        const vector<Transition> &transitions = transitions_by_label[representative_label_no];
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &trans = transitions[j];
            backward_graph[trans.target].push_back(trans.src);
        }
    }

    deque<AbstractStateRef> queue;
    for (AbstractStateRef state = 0; state < num_states; ++state) {
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
        for (size_t i = 0; i < graph[state].size(); ++i) {
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

void TransitionSystem::compute_init_distances_general_cost() {
    vector<vector<pair<int, int> > > forward_graph(num_states);
    for (BlockListConstIter it = equivalent_labels->begin();
         it != equivalent_labels->end(); ++it) {
        const Block &block = *it;
        int representative_label_no = *block.begin();
        int label_cost = labels->get_label_cost(representative_label_no);
        const vector<Transition> &transitions = transitions_by_label[representative_label_no];
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &trans = transitions[j];
            forward_graph[trans.src].push_back(
                make_pair(trans.target, label_cost));
        }
    }

    // TODO: Reuse the same queue for multiple computations to save speed?
    //       Also see compute_goal_distances_general_cost.
    AdaptiveQueue<int> queue;
    for (AbstractStateRef state = 0; state < num_states; ++state) {
        if (state == init_state) {
            init_distances[state] = 0;
            queue.push(0, state);
        }
    }
    dijkstra_search(forward_graph, queue, init_distances);
}

void TransitionSystem::compute_goal_distances_general_cost() {
    vector<vector<pair<int, int> > > backward_graph(num_states);
    for (BlockListConstIter it = equivalent_labels->begin();
         it != equivalent_labels->end(); ++it) {
        const Block &block = *it;
        int representative_label_no = *block.begin();
        int label_cost = labels->get_label_cost(representative_label_no);
        const vector<Transition> &transitions = transitions_by_label[representative_label_no];
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &trans = transitions[j];
            backward_graph[trans.target].push_back(
                make_pair(trans.src, label_cost));
        }
    }

    // TODO: Reuse the same queue for multiple computations to save speed?
    //       Also see compute_init_distances_general_cost.
    AdaptiveQueue<int> queue;
    for (AbstractStateRef state = 0; state < num_states; ++state) {
        if (goal_states[state]) {
            goal_distances[state] = 0;
            queue.push(0, state);
        }
    }
    dijkstra_search(backward_graph, queue, goal_distances);
}

bool TransitionSystem::are_transitions_sorted_unique() const {
    /*
      Determine whether the transitions are sorted uniquely or not.
      (Currently used after construction and shrinking.)

      Note that if this method is called after shrinking or construction of
      a composite transition system, transitions_by_label has just been
      computed from scratch and no equivalence relation is known, hence we
      need to iterate over all labels, collecting transitions.
    */
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        if (labels->is_current_label(label_no)) {
            const vector<Transition> &transitions = transitions_by_label[label_no];
            if (!is_sorted_unique(transitions)) {
                return false;
            }
        }
    }
    return true;
}

void TransitionSystem::normalize_transitions() {
    /*
      This method sorts all transitions and removes duplicate transitions.
      Note that currently we do not detect the case where a label
      becomes irrelevant due to shrinking. This could be a future
      optimization.

      Note that when this method is called, transitions_by_label has been
      computed from scratch and no equivalence relation is known, hence we
      need to iterate over all labels, collecting transitions.
    */

    assert(!are_transitions_sorted_unique());
    assert(!are_equivalent_labels_computed());
    //cout << tag() << "normalizing" << endl;

    typedef vector<pair<AbstractStateRef, int> > StateBucket;

    // First, partition by target state.
    vector<StateBucket> target_buckets(num_states);

    for (int label_no = 0; label_no < num_labels; ++label_no) {
        if (labels->is_current_label(label_no)) {
            vector<Transition> &transitions = transitions_by_label[label_no];
            for (size_t i = 0; i < transitions.size(); ++i) {
                const Transition &t = transitions[i];
                target_buckets[t.target].push_back(
                    make_pair(t.src, label_no));
            }
            vector<Transition>().swap(transitions);
        }
    }

    // Second, partition by src state.
    vector<StateBucket> src_buckets(num_states);

    for (AbstractStateRef target = 0; target < num_states; ++target) {
        StateBucket &bucket = target_buckets[target];
        for (size_t i = 0; i < bucket.size(); ++i) {
            AbstractStateRef src = bucket[i].first;
            int label_no = bucket[i].second;
            src_buckets[src].push_back(make_pair(target, label_no));
        }
    }
    vector<StateBucket>().swap(target_buckets);

    // Finally, partition by label and drop duplicates.
    for (AbstractStateRef src = 0; src < num_states; ++src) {
        StateBucket &bucket = src_buckets[src];
        for (size_t i = 0; i < bucket.size(); ++i) {
            int target = bucket[i].first;
            int label_no = bucket[i].second;

            vector<Transition> &op_bucket = transitions_by_label[label_no];
            Transition trans(src, target);
            if (op_bucket.empty() || op_bucket.back() != trans)
                op_bucket.push_back(trans);
        }
    }

    assert(are_transitions_sorted_unique());
}

bool TransitionSystem::is_label_reduced() const {
    return num_labels == labels->get_size();
}

void TransitionSystem::replace_labels_by_locally_equivalent_one(
    int new_label_no,
    const std::vector<int> &old_label_nos) {
    /*
      Here we handle those transitions systems for which we know that
      only locally equivalent labels are combined. We simply move the
      transitions of the old representative of the reduced labels to
      the new representative of the reduced and new labels. Note that
      this representative may not have changed.
    */

    // Update the equivalence relation.
    int new_representative = equivalent_labels->replace_elements_by_new_one(
        old_label_nos, new_label_no);

    int old_representative = label_to_representative[old_label_nos[0]];
    if (new_representative != old_representative) {
        /*
          If the previous representative of the block from which all
          reduced labels stem has been reduced, we need to move its
          transitions to the new representative and update the label to
          representative mapping.
        */
        vector<Transition> &new_representative_transitions =
            transitions_by_label[new_representative];
        assert(new_representative_transitions.empty());
        new_representative_transitions.swap(transitions_by_label[old_representative]);
        BlockListConstIter block = equivalent_labels->get_block_iterator_for_element(new_representative);
        for (ElementListConstIter elem_it = block->begin();
             elem_it != block->end(); ++elem_it) {
            label_to_representative[*elem_it] = new_representative;
        }
    }
    relevant_labels[new_label_no] = relevant_labels[old_representative];
    label_to_representative[new_label_no] = new_representative;
}

void TransitionSystem::apply_general_label_mapping(int new_label_no,
                                                   const std::vector<int> &old_label_nos) {
    /*
      Some complications arise when we combine labels of which some
      are relevant and others are not. In this case, we test if all
      transitions of relevant labels are self-loops. If this happens,
      we make the new label irrelevant. Otherwise, the new label is
      relevant and we must materialize all previously implicit
      self-loops.
    */

    // new_transitions collects all transitions from all old labels
    set<Transition> new_transitions;
    bool some_parent_is_irrelevant = false;
    bool all_transitions_are_self_loops = true;
    for (size_t j = 0; j < old_label_nos.size(); ++j) {
        int old_label_no = old_label_nos[j];
        const vector<Transition> &old_transitions = get_transitions_for_label(old_label_no);
        if (relevant_labels[old_label_no]) {
            new_transitions.insert(old_transitions.begin(), old_transitions.end());
            if (all_transitions_are_self_loops) {
                for (size_t k = 0; k < old_transitions.size(); ++k) {
                    const Transition &t = old_transitions[k];
                    if (t.target != t.src) {
                        all_transitions_are_self_loops = false;
                        break;
                    }
                }
            }
        } else {
            some_parent_is_irrelevant = true;
        }
    }
    if (some_parent_is_irrelevant) {
        if (all_transitions_are_self_loops) {
            // new label is irrelevant (implicit self-loops)
            relevant_labels[new_label_no] = false;
            // remove all transitions
            set<Transition>().swap(new_transitions);
        } else {
            // new label is relevant
            relevant_labels[new_label_no] = true;
            // make self loops explicit
            for (int i = 0; i < num_states; ++i) {
                new_transitions.insert(Transition(i, i));
            }
        }
    } else {
        // new label is relevant
        relevant_labels[new_label_no] = true;
    }
    vector<Transition> &transitions = transitions_by_label[new_label_no];
    assert(transitions.empty());
    transitions.reserve(new_transitions.size());
    transitions.assign(new_transitions.begin(), new_transitions.end());
}

bool TransitionSystem::are_equivalent_labels_computed() const {
    return equivalent_labels != 0;
}

void TransitionSystem::compute_local_equivalence_relation() {
    /*
      If label l1 is irrelevant and label l2 is relevant but has exactly the
      transitions of an irrelevant label, we do not detect the equivalence.
    */
    assert(!equivalent_labels);
    assert(are_transitions_sorted_unique());

    /*
      Overwrite all entries of label_to_representative with -1 to make sure
      that also when shrinking, the content is properly reset before assiging
      new representatives below.
    */
    label_to_representative.assign(g_operators.empty() ? 0 : g_operators.size() * 2 - 1, -1);

    vector<bool> considered_labels(num_labels, false);
    vector<pair<int, int> > annotated_labels;
    int annotation = 0;
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        if (labels->is_current_label(label_no)) {
            if (considered_labels[label_no]) {
                continue;
            }
            int label_cost = labels->get_label_cost(label_no);
            annotated_labels.push_back(make_pair(annotation, label_no));
            const vector<Transition> &transitions = transitions_by_label[label_no];
            for (int other_label_no = label_no + 1; other_label_no < num_labels;
                 ++other_label_no) {
                if (labels->is_current_label(other_label_no)) {
                    if (considered_labels[other_label_no]) {
                        continue;
                    }
                    if (label_cost != labels->get_label_cost(other_label_no)) {
                        continue;
                    }
                    if (relevant_labels[label_no] != relevant_labels[other_label_no]) {
                        continue;
                    }
                    const vector<Transition> &other_transitions
                            = transitions_by_label[other_label_no];
                    if ((transitions.empty() && other_transitions.empty())
                        || (transitions == other_transitions)) {
                        considered_labels[other_label_no] = true;
                        annotated_labels.push_back(make_pair(annotation, other_label_no));
                    }
                }
            }
            ++annotation;
        }
    }
    equivalent_labels = EquivalenceRelation::from_annotated_elements<int>(num_labels, annotated_labels);

    /*
      Go over the equivalence relation and delete all transitions of labels
      which are represented by another, locally equivalent label. Set
      label_to_representative accordingly.
    */
    for (BlockListConstIter it = equivalent_labels->begin();
         it != equivalent_labels->end(); ++it) {
        const Block &block = *it;
        int min_label_no = *block.begin();
        label_to_representative[min_label_no] = min_label_no;
        for (ElementListConstIter jt = block.begin(); jt != block.end(); ++jt) {
            assert(*jt < num_labels);
            if (jt == block.begin())
                continue;
            int label_no = *jt;
            assert(min_label_no < label_no);
            vector<Transition>().swap(transitions_by_label[label_no]);
            label_to_representative[label_no] = min_label_no;
        }
    }
}

void TransitionSystem::build_atomic_transition_systems(vector<TransitionSystem *> &result,
                                                       Labels *labels,
                                                       OperatorCost cost_type) {
    assert(result.empty());
    cout << "Building atomic transition systems... " << endl;
    int var_count = g_variable_domain.size();

    // Step 1: Create the transition system objects without transitions.
    for (int var_no = 0; var_no < var_count; ++var_no)
        result.push_back(new AtomicTransitionSystem(labels, var_no));

    // Step 2: Add transitions.
    int op_count = g_operators.size();
    for (int label_no = 0; label_no < op_count; ++label_no) {
        const GlobalOperator &op = g_operators[label_no];
        labels->add_label(get_adjusted_action_cost(op, cost_type));
        const vector<GlobalCondition> &preconditions = op.get_preconditions();
        const vector<GlobalEffect> &effects = op.get_effects();
        hash_map<int, int> pre_val;
        vector<bool> has_effect_on_var(var_count, false);
        for (size_t i = 0; i < preconditions.size(); ++i)
            pre_val[preconditions[i].var] = preconditions[i].val;

        for (size_t i = 0; i < effects.size(); ++i) {
            int var = effects[i].var;
            has_effect_on_var[var] = true;
            int post_value = effects[i].val;
            TransitionSystem *ts = result[var];

            // Determine possible values that var can have when this
            // operator is applicable.
            int pre_value = -1;
            hash_map<int, int>::const_iterator pre_val_it = pre_val.find(var);
            if (pre_val_it != pre_val.end())
                pre_value = pre_val_it->second;
            int pre_value_min, pre_value_max;
            if (pre_value == -1) {
                pre_value_min = 0;
                pre_value_max = g_variable_domain[var];
            } else {
                pre_value_min = pre_value;
                pre_value_max = pre_value + 1;
            }

            /*
              cond_effect_pre_value == x means that the effect has an
              effect condition "var == x".
              cond_effect_pre_value == -1 means no effect condition on var.
              has_other_effect_cond is true iff there exists an effect
              condition on a variable other than var.
            */
            const vector<GlobalCondition> &eff_cond = effects[i].conditions;
            int cond_effect_pre_value = -1;
            bool has_other_effect_cond = false;
            for (size_t j = 0; j < eff_cond.size(); ++j) {
                if (eff_cond[j].var == var) {
                    cond_effect_pre_value = eff_cond[j].val;
                } else {
                    has_other_effect_cond = true;
                }
            }

            // Handle transitions that occur when the effect triggers.
            for (int value = pre_value_min; value < pre_value_max; ++value) {
                /* Only add a transition if it is possible that the effect
                   triggers. We can rule out that the effect triggers if it has
                   a condition on var and this condition is not satisfied. */
                if (cond_effect_pre_value == -1 || cond_effect_pre_value == value) {
                    Transition trans(value, post_value);
                    ts->transitions_by_label[label_no].push_back(trans);
                }
            }

            // Handle transitions that occur when the effect does not trigger.
            if (!eff_cond.empty()) {
                for (int value = pre_value_min; value < pre_value_max; ++value) {
                    /* Add self-loop if the effect might not trigger.
                       If the effect has a condition on another variable, then
                       it can fail to trigger no matter which value var has.
                       If it only has a condition on var, then the effect
                       fails to trigger if this condition is false. */
                    if (has_other_effect_cond || value != cond_effect_pre_value) {
                        Transition loop(value, value);
                        ts->transitions_by_label[label_no].push_back(loop);
                    }
                }
            }

            ts->relevant_labels[label_no] = true;
        }
        for (size_t i = 0; i < preconditions.size(); ++i) {
            int var = preconditions[i].var;
            if (!has_effect_on_var[var]) {
                int value = preconditions[i].val;
                TransitionSystem *ts = result[var];
                Transition trans(value, value);
                ts->transitions_by_label[label_no].push_back(trans);
                ts->relevant_labels[label_no] = true;
            }
        }
    }

    for (size_t i = 0; i < result.size(); ++i) {
        // Need to set the correct number of labels *after* generating them
        result[i]->num_labels = labels->get_size();
        result[i]->compute_local_equivalence_relation();
        result[i]->compute_distances_and_prune();
        assert(result[i]->is_valid());
    }
}

void TransitionSystem::apply_abstraction(
    vector<slist<AbstractStateRef> > &collapsed_groups) {
    assert(is_valid());

    if (static_cast<int>(collapsed_groups.size()) == get_size()) {
        cout << tag() << "not applying abstraction (same number of states)" << endl;
        return;
    }

    cout << tag() << "applying abstraction (" << get_size()
         << " to " << collapsed_groups.size() << " states)" << endl;

    typedef slist<AbstractStateRef> Group;

    vector<int> abstraction_mapping(num_states, PRUNED_STATE);

    for (size_t group_no = 0; group_no < collapsed_groups.size(); ++group_no) {
        Group &group = collapsed_groups[group_no];
        for (Group::iterator pos = group.begin(); pos != group.end(); ++pos) {
            AbstractStateRef state = *pos;
            assert(abstraction_mapping[state] == PRUNED_STATE);
            abstraction_mapping[state] = group_no;
        }
    }

    int new_num_states = collapsed_groups.size();
    vector<int> new_init_distances(new_num_states, INF);
    vector<int> new_goal_distances(new_num_states, INF);
    vector<bool> new_goal_states(new_num_states, false);

    bool must_clear_distances = false;
    for (AbstractStateRef new_state = 0; new_state < new_num_states; ++new_state) {
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

    vector<vector<Transition> > new_transitions_by_label(
        transitions_by_label.size());
    for (BlockListConstIter it = equivalent_labels->begin();
         it != equivalent_labels->end(); ++it) {
        const Block &block = *it;
        int representative_label_no = *block.begin();
        const vector<Transition> &transitions =
            transitions_by_label[representative_label_no];
        for (ElementListConstIter jt = block.begin(); jt != block.end(); ++jt) {
            int label_no = *jt;
            vector<Transition> &new_transitions =
                new_transitions_by_label[label_no];
            new_transitions.reserve(transitions.size());
            for (size_t i = 0; i < transitions.size(); ++i) {
                const Transition &trans = transitions[i];
                int src = abstraction_mapping[trans.src];
                int target = abstraction_mapping[trans.target];
                if (src != PRUNED_STATE && target != PRUNED_STATE)
                    new_transitions.push_back(Transition(src, target));
            }
        }
    }
    vector<vector<Transition> >().swap(transitions_by_label);

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

    /*
      Observation from some experiments in the context of making transition
      systems always be valid: including the normalize below or not
      drastically seems to influence memory usage. Memory usage increases a
      lot if normalizing here after every shrink. The reason for this is
      somewhat unclear.
    */

    /*
      TODO: there is a dangerous dependency on the order in which things are
      done here: compute_distances relies on an existing equivalence relation,
      hence after shrinking and normalizing, we currently *first* have to
      compute the new equivalence relations before computing distances. This
      makes sense on the one hand, but on the other hand, the computation of
      distances may trigger another round of shrinking, normalizing, computation
      of equivalence relation etc.
    */
    if (equivalent_labels) {
        delete equivalent_labels;
        equivalent_labels = 0;
    }
    // TODO do not check if transitions are sorted but just assume they are not?
    if (!are_transitions_sorted_unique()) {
        normalize_transitions();
    }
    compute_local_equivalence_relation();
    if (must_clear_distances) {
        compute_distances_and_prune();
    }
    assert(is_valid());
}

void TransitionSystem::apply_label_reduction(const vector<pair<int, vector<int> > > &label_mapping,
                                             bool only_equivalent_labels) {
    assert(!is_label_reduced());
    assert(are_distances_computed());
    assert(are_transitions_sorted_unique());
    assert(are_equivalent_labels_computed());
    //cout << tag() << " applying label mapping" << endl;
//    bool debug = (varset.size() == 1 && varset[0] == 4);
    bool debug = false;
    if (debug) {
        cout << tag() << " label reduction" << endl;
        cout << "label to representative: " << label_to_representative << endl;
//        dump_transitions();
        equivalent_labels->dump();
    }
    // Go over the mapping of reduced labels to new label one by one.
    for (size_t i = 0; i < label_mapping.size(); ++i) {
        const vector<int> &old_label_nos = label_mapping[i].second;
        assert(old_label_nos.size() >= 2);
        int new_label_no = label_mapping[i].first;
        assert(new_label_no == num_labels);
        if (debug)
            cout << "combining " << old_label_nos << " into " << new_label_no << endl;

        // First, assert that all old labels are in the correct range
        for (size_t j = 0; j < old_label_nos.size(); ++j) {
            assert(old_label_nos[j] < num_labels);
        }

        // Second, handle the actual label reduction.
        if (only_equivalent_labels) {
            /*
              If all reduced labels are locally equivalent, then the new label
              must have the same transition as those reduced ones.

              Add the new label to the equivalence relation. Take care of the
              case that transitions must be moved from a reduced representative
              label to a new one. Also update the label to representative
              mapping.
            */
            replace_labels_by_locally_equivalent_one(new_label_no, old_label_nos);
            if (debug) {
                cout << "after update:" << endl;
                equivalent_labels->dump();
            }
        } else {
            /*
              Here we handle those label reductions for which not all
              reduced labels have necessarily been locally equivalent before.
            */
            // Compute transitions of the new label.
            apply_general_label_mapping(new_label_no, old_label_nos);
            // For now, only remove reduced labels from the equivalence relation
            equivalent_labels->remove_elements(old_label_nos);
        }

        /*
          Third, set all reduced labels to irrelevant and set their
          representative to the dummy value -1.

          Note that reduced labels must be marked as irrelevant in order to
          avoid confusion between reduced labels and labels with empty
          transitions when computing a composite transition system.
        */
        for (size_t j = 0; j < old_label_nos.size(); ++j) {
            int old_label_no = old_label_nos[j];
            label_to_representative[old_label_no] = -1;
            relevant_labels[old_label_no] = false;
        }
        ++num_labels;
    }

    if (!only_equivalent_labels) {
        /*
          If reducing labels which have not necessarily been locally equivalent
          before, we still need to do the following:
           - For all reduced labels that have been representatives, we must
             move their transitions to the new representative (if the group
             of locally equivalent labels did not become empty) and update the
             label to representative mapping.
           - For all new labels (whose transitions we computed above), we must
             check if they are locally equivalent to existing or other new
             labels and update the equivalence relation and the label to
             representative mapping.
           - We must remove all left over transitions of reduced labels (this
             can happen if the entire group of locally equivalent labels has
             been reduced)
        */

        /*
          Go over the equivalence relation and check for all representatives
          if they have been a representative before. If so, move the
          transitions of the previous (now reduced) representative to the new
          one. Update the label to representative mapping accordingly.
        */
        for (BlockListConstIter it = equivalent_labels->begin();
             it != equivalent_labels->end(); ++it) {
            const Block &block = *it;
            int min_label_no = *block.begin();
            if (label_to_representative[min_label_no] != min_label_no) {
                if (debug) {
                    cout << "new repr " << min_label_no << " used to be "
                            "represtend by " << label_to_representative[min_label_no] << endl;
                }
                transitions_by_label[min_label_no].swap(transitions_by_label[label_to_representative[min_label_no]]);
                label_to_representative[min_label_no] = min_label_no;
                for (ElementListConstIter jt = block.begin(); jt != block.end(); ++jt) {
                    assert(*jt < num_labels);
                    if (jt == block.begin())
                        continue;
                    int label_no = *jt;
                    assert(min_label_no < label_no);
                    assert(transitions_by_label[label_no].empty());
                    label_to_representative[label_no] = min_label_no;
                }
            }
        }

        /*
          For every new label, check if its transitions are locally equivalent
          to those of an existing label or of another new label. Update the
          equivalence relation and the label to representative mapping
          accordingly. Delete transitions of new labels that are
          represented by another one and also any left over transitions of
          reduced labels.
        */
        for (size_t i = 0; i < label_mapping.size(); ++i) {
            int new_label_no = label_mapping[i].first;
            int new_label_cost = labels->get_label_cost(new_label_no);
            bool new_label_relevant = relevant_labels[new_label_no];
            const vector<Transition> &new_transitions = transitions_by_label[new_label_no];
            int new_labels_representative = -1;
            for (BlockListConstIter block_it = equivalent_labels->begin();
                 block_it != equivalent_labels->end(); ++block_it) {
                int representative_label_no = *block_it->begin();
                if (labels->get_label_cost(representative_label_no) != new_label_cost)
                    continue;
                if (relevant_labels[representative_label_no] != new_label_relevant)
                    continue;
                const vector<Transition> &other_transitions = transitions_by_label[representative_label_no];
                if ((new_transitions.empty() && other_transitions.empty())
                    || (new_transitions == other_transitions)) {
                    new_labels_representative = representative_label_no;
                    break;
                }
            }
            equivalent_labels->insert(new_label_no, new_labels_representative);
            if (new_labels_representative == -1) {
                new_labels_representative = new_label_no;
            } else {
                /*
                  The new label is locally equivalent to another label,
                  hence we delete its transitions.
                */
                vector<Transition>().swap(transitions_by_label[new_label_no]);
            }
            label_to_representative[new_label_no] = new_labels_representative;

            // Remove left over transitions of reduced labels
            const vector<int> &old_label_nos = label_mapping[i].second;
            for (size_t j = 0; j < old_label_nos.size(); ++j) {
                int old_label_no = old_label_nos[j];
                vector<Transition>().swap(transitions_by_label[old_label_no]);
            }
        }

        if (debug) {
            cout << "after update:" << endl;
            equivalent_labels->dump();
        }
    }

    // NOTE: as we currently only combine labels of the same cost, we do not
    // need to recompute distances after label reduction.
    if (debug) {
        cout << "label to representative: " << label_to_representative << endl;
//        dump_transitions();
        equivalent_labels->dump();
    }
    assert(is_valid());
}

void TransitionSystem::release_memory() {
    vector<bool>().swap(relevant_labels);
    vector<vector<Transition> >().swap(transitions_by_label);
}

string TransitionSystem::tag() const {
    string desc(description());
    desc[0] = toupper(desc[0]);
    return desc + ": ";
}

bool TransitionSystem::is_solvable() const {
    assert(are_distances_computed());
    return init_state != PRUNED_STATE;
}

int TransitionSystem::get_cost(const GlobalState &state) const {
    assert(are_distances_computed());
    int abs_state = get_abstract_state(state);
    if (abs_state == PRUNED_STATE)
        return -1;
    int cost = goal_distances[abs_state];
    assert(cost != INF);
    return cost;
}

int TransitionSystem::memory_estimate() const {
    int result = sizeof(TransitionSystem);
    result += sizeof(Label *) * relevant_labels.capacity();
    result += sizeof(vector<Transition> )
              * transitions_by_label.capacity();
    for (size_t i = 0; i < transitions_by_label.size(); ++i)
        result += sizeof(Transition) * transitions_by_label[i].capacity();
    result += sizeof(int) * init_distances.capacity();
    result += sizeof(int) * goal_distances.capacity();
    result += sizeof(bool) * goal_states.capacity();
    return result;
}

int TransitionSystem::total_transitions() const {
    int total = 0;
    for (size_t i = 0; i < transitions_by_label.size(); ++i)
        total += transitions_by_label[i].size();
    return total;
}

int TransitionSystem::unique_unlabeled_transitions() const {
    vector<Transition> unique_transitions;
    for (size_t i = 0; i < transitions_by_label.size(); ++i) {
        const vector<Transition> &trans = transitions_by_label[i];
        unique_transitions.insert(unique_transitions.end(), trans.begin(),
                                  trans.end());
    }
    ::sort(unique_transitions.begin(), unique_transitions.end());
    return unique(unique_transitions.begin(), unique_transitions.end())
           - unique_transitions.begin();
}

void TransitionSystem::statistics(bool include_expensive_statistics) const {
    int memory = memory_estimate();
    peak_memory = max(peak_memory, memory);
    cout << tag() << get_size() << " states, ";
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
        cout << "transition system is unsolvable";
    }
    cout << " [t=" << g_timer << "]" << endl;
}

int TransitionSystem::get_peak_memory_estimate() const {
    return peak_memory;
}

void TransitionSystem::dump_attributes() const {
    cout << "labels->get_size() " << labels->get_size() << endl;
    cout << "num_labels: " << num_labels << endl;
    //transitions_by_label;
    cout << "total_transitions: " << total_transitions() << endl;
    //relevant_labels;
    cout << "num_states: " << num_states << endl;
    //init_distances;
    //goal_distances;
    //goal_states;
    cout << "init_state: " << init_state << endl;
    cout << "max_f: " << max_f << endl;
    cout << "max_g: " << max_g << endl;
    cout << "max_h: " << max_h << endl;
    cout << "transitions_sorted_unique: " << transitions_sorted_unique << endl;
    cout << "goal_relevant:" << goal_relevant << endl;
    cout << "peak_memory: " << peak_memory << endl;
    cout << "varset: " << varset << endl;
}

void TransitionSystem::dump_dot_graph() const {
    cout << "digraph transition system";
    for (size_t i = 0; i < varset.size(); ++i)
        cout << "_" << varset[i];
    cout << " {" << endl;
    cout << "    node [shape = none] start;" << endl;
    for (int i = 0; i < num_states; ++i) {
        bool is_init = (i == init_state);
        bool is_goal = (goal_states[i] == true);
        cout << "    node [shape = " << (is_goal ? "doublecircle" : "circle")
             << "] node" << i << ";" << endl;
        if (is_init)
            cout << "    start -> node" << i << ";" << endl;
    }
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        // reduced labels are automatically skipped because trans is then empty
        const vector<Transition> &trans = get_transitions_for_label(label_no);
        for (size_t i = 0; i < trans.size(); ++i) {
            int src = trans[i].src;
            int target = trans[i].target;
            cout << "    node" << src << " -> node" << target << " [label = o_"
                 << label_no << "];" << endl;
        }
    }
    cout << "}" << endl;
}

void TransitionSystem::dump_transitions() const {
    cout << tag() << "transitions" << endl;
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        // reduced labels are automatically skipped because trans is then empty
        const vector<Transition> &trans = transitions_by_label[label_no];
        for (size_t i = 0; i < trans.size(); ++i) {
            int src = trans[i].src;
            int target = trans[i].target;
            cout << src << " -> " << target << " label: " << label_no << endl;
        }
    }
}

void TransitionSystem::dump_equivalence_relation() const {
    cout << tag() << endl;
    equivalent_labels->dump();
}

void TransitionSystem::compute_label_ranks(vector<int> &label_ranks) const {
    assert(is_valid());
    assert(label_ranks.empty());
    label_ranks.reserve(transitions_by_label.size());
    for (size_t label_no = 0; label_no < transitions_by_label.size(); ++label_no) {
        if (relevant_labels[label_no]) {
            const vector<Transition> &transitions = get_transitions_for_label(label_no);
            int label_rank = INF;
            for (size_t i = 0; i < transitions.size(); ++i) {
                const Transition &t = transitions[i];
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

const vector<Transition> &TransitionSystem::get_transitions_for_label(int label_no) const {
    int representative = label_to_representative[label_no];
    if (representative == -1) {
        std::cout << "found representative -1 while looking up " << label_no << std::endl;
        assert(false);
    }
    return transitions_by_label[representative];
}



AtomicTransitionSystem::AtomicTransitionSystem(Labels *labels, int variable_)
    : TransitionSystem(labels), variable(variable_) {
    varset.push_back(variable);
    /*
      This generates the states of the atomic transition system, but not the
      arcs: It is more efficient to generate all arcs of all atomic
      transition systems simultaneously.
     */
    int range = g_variable_domain[variable];

    int init_value = g_initial_state()[variable];
    int goal_value = -1;
    goal_relevant = false;
    for (size_t goal_no = 0; goal_no < g_goal.size(); ++goal_no) {
        if (g_goal[goal_no].first == variable) {
            goal_relevant = true;
            assert(goal_value == -1);
            goal_value = g_goal[goal_no].second;
        }
    }

    num_states = range;
    lookup_table.reserve(range);
    goal_states.resize(num_states, false);
    for (int value = 0; value < range; ++value) {
        if (value == goal_value || goal_value == -1) {
            goal_states[value] = true;
        }
        if (value == init_value)
            init_state = value;
        lookup_table.push_back(value);
    }
}

AtomicTransitionSystem::~AtomicTransitionSystem() {
}

void AtomicTransitionSystem::apply_abstraction_to_lookup_table(
    const vector<AbstractStateRef> &abstraction_mapping) {
    cout << tag() << "applying abstraction to lookup table" << endl;
    for (size_t i = 0; i < lookup_table.size(); ++i) {
        AbstractStateRef old_state = lookup_table[i];
        if (old_state != PRUNED_STATE)
            lookup_table[i] = abstraction_mapping[old_state];
    }
}

string AtomicTransitionSystem::description() const {
    ostringstream s;
    s << "atomic transition system #" << variable;
    return s.str();
}

AbstractStateRef AtomicTransitionSystem::get_abstract_state(const GlobalState &state) const {
    int value = state[variable];
    return lookup_table[value];
}

int AtomicTransitionSystem::memory_estimate() const {
    int result = TransitionSystem::memory_estimate();
    result += sizeof(AtomicTransitionSystem) - sizeof(TransitionSystem);
    result += sizeof(AbstractStateRef) * lookup_table.capacity();
    return result;
}



CompositeTransitionSystem::CompositeTransitionSystem(Labels *labels,
                                                     TransitionSystem *ts1,
                                                     TransitionSystem *ts2)
    : TransitionSystem(labels) {
    cout << "Merging " << ts1->description() << " and "
         << ts2->description() << endl;

    assert(ts1->is_solvable() && ts2->is_solvable());
    assert(ts1->is_valid() && ts2->is_valid());

    components[0] = ts1;
    components[1] = ts2;

    ::set_union(ts1->varset.begin(), ts1->varset.end(), ts2->varset.begin(),
                ts2->varset.end(), back_inserter(varset));

    int ts1_size = ts1->get_size();
    int ts2_size = ts2->get_size();
    num_states = ts1_size * ts2_size;
    goal_states.resize(num_states, false);
    goal_relevant = (ts1->goal_relevant || ts2->goal_relevant);

    lookup_table.resize(ts1->get_size(), vector<AbstractStateRef> (ts2->get_size()));
    for (int s1 = 0; s1 < ts1_size; ++s1) {
        for (int s2 = 0; s2 < ts2_size; ++s2) {
            int state = s1 * ts2_size + s2;
            lookup_table[s1][s2] = state;
            if (ts1->goal_states[s1] && ts2->goal_states[s2])
                goal_states[state] = true;
            if (s1 == ts1->init_state && s2 == ts2->init_state)
                init_state = state;
        }
    }

    /* Note:
       The way we construct the transitions of the new composite transition system,
       we cannot easily guarantee that they are sorted. Given that we have
       transitions a->b and c->d in transition system one and two, we would like to
       have (a,c)->(b,d) in the resultin transition system. There is no obvious way
       at looking at the transitions of transition systems one and two that would
       result in the desired ordering. Even in the case that the second
       transition systems has only self loops, this is not trivial, as we would like
       to sort transitions to (a,c,b). Only in the case that the first
       transition system has only self-lopos, by looking at each transition of the
       first transition system and multiplying in out with the transitions of the
       second transition, we obtain the desired order (a,c,d).
     */
    int multiplier = ts2_size;
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        bool relevant1 = ts1->relevant_labels[label_no];
        bool relevant2 = ts2->relevant_labels[label_no];
        if (relevant1 || relevant2) {
            relevant_labels[label_no] = true;
            vector<Transition> &transitions = transitions_by_label[label_no];
            const vector<Transition> &bucket1 =
                ts1->get_transitions_for_label(label_no);
            const vector<Transition> &bucket2 =
                ts2->get_transitions_for_label(label_no);
            if (relevant1 && relevant2) {
                if (bucket1.size() * bucket2.size() > transitions.max_size())
                    exit_with(EXIT_OUT_OF_MEMORY);
                transitions.reserve(bucket1.size() * bucket2.size());
                for (size_t i = 0; i < bucket1.size(); ++i) {
                    int src1 = bucket1[i].src;
                    int target1 = bucket1[i].target;
                    for (size_t j = 0; j < bucket2.size(); ++j) {
                        int src2 = bucket2[j].src;
                        int target2 = bucket2[j].target;
                        int src = src1 * multiplier + src2;
                        int target = target1 * multiplier + target2;
                        transitions.push_back(Transition(src, target));
                    }
                }
            } else if (relevant1) {
                assert(!relevant2);
                if (bucket1.size() * ts2_size > transitions.max_size())
                    exit_with(EXIT_OUT_OF_MEMORY);
                transitions.reserve(bucket1.size() * ts2_size);
                for (size_t i = 0; i < bucket1.size(); ++i) {
                    int src1 = bucket1[i].src;
                    int target1 = bucket1[i].target;
                    for (int s2 = 0; s2 < ts2_size; ++s2) {
                        int src = src1 * multiplier + s2;
                        int target = target1 * multiplier + s2;
                        transitions.push_back(Transition(src, target));
                    }
                }
            } else if (relevant2) {
                assert(!relevant1);
                if (bucket2.size() * ts1_size > transitions.max_size())
                    exit_with(EXIT_OUT_OF_MEMORY);
                transitions.reserve(bucket2.size() * ts1_size);
                for (int s1 = 0; s1 < ts1_size; ++s1) {
                    for (size_t i = 0; i < bucket2.size(); ++i) {
                        int src2 = bucket2[i].src;
                        int target2 = bucket2[i].target;
                        int src = s1 * multiplier + src2;
                        int target = s1 * multiplier + target2;
                        transitions.push_back(Transition(src, target));
                    }
                }
                assert(is_sorted_unique(transitions));
            }
        }
    }

    // TODO do not check if transitions are sorted but just assume they are not?
    if (!are_transitions_sorted_unique()) {
        normalize_transitions();
    }
    compute_local_equivalence_relation();
    compute_distances_and_prune();
    assert(is_valid());
}

CompositeTransitionSystem::~CompositeTransitionSystem() {
}

void CompositeTransitionSystem::apply_abstraction_to_lookup_table(
    const vector<AbstractStateRef> &abstraction_mapping) {
    cout << tag() << "applying abstraction to lookup table" << endl;
    for (int i = 0; i < components[0]->get_size(); ++i) {
        for (int j = 0; j < components[1]->get_size(); ++j) {
            AbstractStateRef old_state = lookup_table[i][j];
            if (old_state != PRUNED_STATE)
                lookup_table[i][j] = abstraction_mapping[old_state];
        }
    }
}

string CompositeTransitionSystem::description() const {
    ostringstream s;
    s << "transition system (" << varset.size() << "/"
      << g_variable_domain.size() << " vars)";
    return s.str();
}

AbstractStateRef CompositeTransitionSystem::get_abstract_state(const GlobalState &state) const {
    AbstractStateRef state1 = components[0]->get_abstract_state(state);
    AbstractStateRef state2 = components[1]->get_abstract_state(state);
    if (state1 == PRUNED_STATE || state2 == PRUNED_STATE)
        return PRUNED_STATE;
    return lookup_table[state1][state2];
}

int CompositeTransitionSystem::memory_estimate() const {
    int result = TransitionSystem::memory_estimate();
    result += sizeof(CompositeTransitionSystem) - sizeof(TransitionSystem);
    result += sizeof(vector<AbstractStateRef> ) * lookup_table.capacity();
    for (size_t i = 0; i < lookup_table.size(); ++i)
        result += sizeof(AbstractStateRef) * lookup_table[i].capacity();
    return result;
}
