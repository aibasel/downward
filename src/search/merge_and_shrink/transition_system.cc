#include "transition_system.h"

#include "labels.h"
#include "shrink_fh.h"

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
#include <functional>
#include <iostream>
#include <fstream>
#include <limits>
#include <set>
#include <string>
#include <sstream>
#include <unordered_map>
using namespace std;

/* Implementation note: Transitions are grouped by thei label groups,
 not by source state or any such thing. Such a grouping is beneficial
 for fast generation of products because we can iterate label group by
 label group, and it also allows applying transition system mappings very
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
    : labels(labels_),
      transitions_by_group_index(g_operators.empty() ? 0 : g_operators.size() * 2 - 1),
      label_to_positions(g_operators.empty() ? 0 : g_operators.size() * 2 - 1),
      num_labels(labels->get_size()),
      peak_memory(0) {
    clear_distances();
}

TransitionSystem::~TransitionSystem() {
}

bool TransitionSystem::is_valid() const {
    return are_distances_computed()
           && are_transitions_sorted_unique()
           && is_label_reduced();
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
    vector<forward_list<AbstractStateRef> > equivalence_relation;
    equivalence_relation.reserve(num_states);
    for (int state = 0; state < num_states; ++state) {
        if (!to_be_pruned_states[state]) {
            forward_list<AbstractStateRef> group;
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
    assert(are_transitions_sorted_unique());
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
        if (labels->is_current_label(label_no)) {
            if (labels->get_label_cost(label_no) != 1) {
                is_unit_cost = false;
                break;
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
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = get_const_transitions_for_group(*group_it);
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
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = get_const_transitions_for_group(*group_it);
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
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = get_const_transitions_for_group(*group_it);
        int cost = get_cost_for_label_group(*group_it);
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &trans = transitions[j];
            forward_graph[trans.src].push_back(
                make_pair(trans.target, cost));
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
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = get_const_transitions_for_group(*group_it);
        int cost = get_cost_for_label_group(*group_it);
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &trans = transitions[j];
            backward_graph[trans.target].push_back(
                make_pair(trans.src, cost));
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

const vector<Transition> &TransitionSystem::get_const_transitions_for_label(int label_no) const {
    int transitions_index = get<0>(label_to_positions[label_no]);
    return transitions_by_group_index[transitions_index];
}

vector<Transition> &TransitionSystem::get_transitions_for_group(const list<int> &group) {
    return transitions_by_group_index[get_transitions_index_for_group(group)];
}

int TransitionSystem::get_transitions_index_for_group(const std::list<int> &group) const {
    assert(!group.empty());
    int any_label_no = group.front();
    int index = get<0>(label_to_positions[any_label_no]);
    assert(index != -1);
    return index;
}

void TransitionSystem::normalize_given_transitions(vector<Transition> &transitions) const {
    sort(transitions.begin(), transitions.end());
    transitions.erase(unique(transitions.begin(), transitions.end()), transitions.end());
}

bool TransitionSystem::are_transitions_sorted_unique() const {
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = get_const_transitions_for_group(*group_it);
        if (!is_sorted_unique(transitions)) {
            return false;
        }
    }
    return true;
}

bool TransitionSystem::is_label_reduced() const {
    return num_labels == labels->get_size();
}

void TransitionSystem::compute_locally_equivalent_labels() {
    /*
      Compare every block of labels and transitions to all others and merge
      two blocks whenever the transitions are the same.
    */
    for (LabelGroupIter group1_it = grouped_labels.begin();
         group1_it != grouped_labels.end(); ++group1_it) {
        const vector<Transition> &trans1 = get_const_transitions_for_group(*group1_it);
        int cost1 = get_cost_for_label_group(*group1_it);
        for (LabelGroupIter group2_it = group1_it;
             group2_it != grouped_labels.end(); ++group2_it) {
            if (group2_it == group1_it)
                continue;
            if (get_cost_for_label_group(*group2_it) == cost1) {
                vector<Transition> &trans2 = get_transitions_for_group(*group2_it);
                if ((trans1.empty() && trans2.empty()) || trans1 == trans2) {
                    for (LabelConstIter group2_label_it = group2_it->begin();
                         group2_label_it != group2_it->end(); ++group2_label_it) {
                        int other_label_no = *group2_label_it;
                        LabelIter label_it = group1_it->insert(group1_it->end(), other_label_no);
                        int group1_index = get_transitions_index_for_group(*group1_it);
                        label_to_positions[other_label_no] = make_tuple(group1_index, group1_it, label_it);
                    }
                    grouped_labels.erase(group2_it);
                    --group2_it;
                    vector<Transition>().swap(trans2);
                }
            }
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
    vector<vector<bool> > relevant_labels(result.size(), vector<bool>(op_count, false));
    for (int label_no = 0; label_no < op_count; ++label_no) {
        const GlobalOperator &op = g_operators[label_no];
        labels->add_label(get_adjusted_action_cost(op, cost_type));
        const vector<GlobalCondition> &preconditions = op.get_preconditions();
        const vector<GlobalEffect> &effects = op.get_effects();
        unordered_map<int, int> pre_val;
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
            const auto pre_val_it = pre_val.find(var);
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
                    ts->transitions_by_group_index[label_no].push_back(trans);
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
                        ts->transitions_by_group_index[label_no].push_back(loop);
                    }
                }
            }

            relevant_labels[var][label_no] = true;
        }
        for (size_t i = 0; i < preconditions.size(); ++i) {
            int var = preconditions[i].var;
            if (!has_effect_on_var[var]) {
                int value = preconditions[i].val;
                TransitionSystem *ts = result[var];
                Transition trans(value, value);
                ts->transitions_by_group_index[label_no].push_back(trans);
                relevant_labels[var][label_no] = true;
            }
        }
    }

    for (size_t i = 0; i < result.size(); ++i) {
        // Need to set the correct number of labels *after* generating them
        result[i]->num_labels = labels->get_size();

        // Make all irrelevant labels explicit and create label_blocks
        for (int label_no = 0; label_no < result[i]->num_labels; ++label_no) {
            TransitionSystem *ts = result[i];
            if (!relevant_labels[i][label_no]) {
                for (int state = 0; state < ts->num_states; ++state) {
                    Transition loop(state, state);
                    ts->transitions_by_group_index[label_no].push_back(loop);
                }
            }
        }
        result[i]->compute_locally_equivalent_labels();
        result[i]->compute_distances_and_prune();
        assert(result[i]->is_valid());
    }
}

void TransitionSystem::apply_abstraction(
    vector<forward_list<AbstractStateRef> > &collapsed_groups) {
    assert(is_valid());

    if (static_cast<int>(collapsed_groups.size()) == get_size()) {
        cout << tag() << "not applying abstraction (same number of states)" << endl;
        return;
    }

    cout << tag() << "applying abstraction (" << get_size()
         << " to " << collapsed_groups.size() << " states)" << endl;

    typedef forward_list<AbstractStateRef> Group;

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

    // Update all transitions. Locally equivalent labels remain locally equivalent.
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        vector<Transition> &transitions = get_transitions_for_group(*group_it);
        vector<Transition> new_transitions;
        for (size_t i = 0; i < transitions.size(); ++i) {
            const Transition &trans = transitions[i];
            int src = abstraction_mapping[trans.src];
            int target = abstraction_mapping[trans.target];
            if (src != PRUNED_STATE && target != PRUNED_STATE)
                new_transitions.push_back(Transition(src, target));
        }
        normalize_given_transitions(new_transitions);
        transitions.swap(new_transitions);
    }

    compute_locally_equivalent_labels();

    num_states = new_num_states;
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

    // Go over the mapping of reduced labels to new label one by one.
    for (size_t i = 0; i < label_mapping.size(); ++i) {
        const vector<int> &old_label_nos = label_mapping[i].second;
        assert(old_label_nos.size() >= 2);
        int new_label_no = label_mapping[i].first;
        assert(new_label_no == num_labels);

        // Assert that all old labels are in the correct range
        for (size_t j = 0; j < old_label_nos.size(); ++j) {
            assert(old_label_nos[j] < num_labels);
        }

        if (only_equivalent_labels) {
            /*
              Here we handle those transitions systems for which we know that
              only locally equivalent labels are combined which means that
              the new label has the same transitions as the reduces ones.
            */

            /*
              Remove all existing labels from their list and remove the entry in
              label_to_group.
            */
            LabelGroupIter canonical_group_it = get<1>(label_to_positions[old_label_nos[0]]);
            int transitions_index = get_transitions_index_for_group(*canonical_group_it);
            for (size_t i = 0; i < old_label_nos.size(); ++i) {
                int label_no = old_label_nos[i];
                assert(get<1>(label_to_positions[label_no]) == canonical_group_it);
                LabelIter label_it = get<2>(label_to_positions[label_no]);
                canonical_group_it->erase(label_it);
                // Note: we cannot invalidate the tupel label_to_positions[label_no]
            }

            /*
              Add the new label to the list and add an entry to label_to_group.
            */
            LabelIter label_it = canonical_group_it->insert(canonical_group_it->end(), new_label_no);
            label_to_positions[new_label_no] = make_tuple(transitions_index, canonical_group_it, label_it);
        } else {
            /*
              Here we handle those label reductions for which not all
              reduced labels have necessarily been locally equivalent before.

              Note that we have to do this in a two-fold manner: first, we
              compute the new transitions from the old ones and delete the
              entries for both the reduced labels and their transitions. In
              a second iteration, we test for all new labels and their
              transitions if they are locally equivalent with others. If we
              did so for every "single" label mapping immediately, we would
              miss the case that new labels can be locally equivalent to each
              other. The alternative would be to perform a "full" computation
              of the computation of locally equivalent labels afterwards.
            */

            // collected_transitions collects all transitions from all old labels
            set<Transition> collected_transitions;
            for (size_t j = 0; j < old_label_nos.size(); ++j) {
                int old_label_no = old_label_nos[j];
                const vector<Transition> &old_transitions = get_const_transitions_for_label(old_label_no);
                collected_transitions.insert(old_transitions.begin(), old_transitions.end());
            }
            transitions_by_group_index[new_label_no].assign(collected_transitions.begin(), collected_transitions.end());

            /*
              Remove all existing labels from their group (and the group itself if
              it becomes empty) and update label_to_positions
            */
            for (size_t i = 0; i < old_label_nos.size(); ++i) {
                int label_no = old_label_nos[i];
                int transitions_index;
                LabelGroupIter group_it;
                LabelIter label_it;
                tie(transitions_index, group_it, label_it) = label_to_positions[label_no];
                group_it->erase(label_it);
                // Note: we cannot invalidate the tupel label_to_positions[label_no]
                if (group_it->empty()) {
                    grouped_labels.erase(group_it);
                    vector<Transition>().swap(transitions_by_group_index[transitions_index]);
                }
            }
        }
        ++num_labels;
    }

    if (!only_equivalent_labels) {
        /*
          For every new label, find existing locally equivalent labels or add
          a new own list for it.
        */
        for (size_t i = 0; i < label_mapping.size(); ++i) {
            int new_label_no = label_mapping[i].first;
            int new_label_cost = labels->get_label_cost(new_label_no);
            const vector<Transition> &new_transitions = transitions_by_group_index[new_label_no];
            bool found_equivalent_labels = false;
            for (LabelGroupIter group_it = grouped_labels.begin();
                 group_it != grouped_labels.end(); ++group_it) {
                if (get_cost_for_label_group(*group_it) != new_label_cost)
                    continue;
                const vector<Transition> &other_transitions = get_const_transitions_for_group(*group_it);
                if ((new_transitions.empty() && other_transitions.empty())
                    || (new_transitions == other_transitions)) {
                    found_equivalent_labels = true;
                    int transitions_index = get_transitions_index_for_group(*group_it);
                    LabelIter label_it = group_it->insert(group_it->end(), new_label_no);
                    label_to_positions[new_label_no] = make_tuple(transitions_index, group_it, label_it);
                    vector<Transition>().swap(transitions_by_group_index[new_label_no]);
                    break;
                }
            }
            if (!found_equivalent_labels) {
                LabelGroupIter group_it = grouped_labels.insert(grouped_labels.end(), list<int>());
                LabelIter label_it = group_it->insert(group_it->end(), new_label_no);
                label_to_positions[new_label_no] = make_tuple(new_label_no, group_it, label_it);
            }
        }
    }

    assert(is_valid());
}

void TransitionSystem::release_memory() {
    list<list<int> >().swap(grouped_labels);
    vector<vector<Transition> >().swap(transitions_by_group_index);
    vector<tuple<int, LabelGroupIter, LabelIter> >().swap(label_to_positions);
}

const vector<Transition> &TransitionSystem::get_const_transitions_for_group(const list<int> &group) const {
    return transitions_by_group_index[get_transitions_index_for_group(group)];
}

int TransitionSystem::get_cost_for_label_group(const list<int> &group) const {
    return labels->get_label_cost(group.front());
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
    result += sizeof(list<int>) * grouped_labels.size();
    result += sizeof(vector<Transition>) * transitions_by_group_index.capacity();
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const list<int> &label_list = *group_it;
        result += sizeof(list<int>) * label_list.size();
        const vector<Transition> &transitions = get_const_transitions_for_group(label_list);
        result += sizeof(vector<Transition>) * transitions.capacity();
    }
    result += sizeof(vector<tuple<int, LabelGroupIter, LabelIter> >) * label_to_positions.capacity();
    result += sizeof(int) * init_distances.capacity();
    result += sizeof(int) * goal_distances.capacity();
    result += sizeof(bool) * goal_states.capacity();
    return result;
}

int TransitionSystem::total_transitions() const {
    int total = 0;
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        total += get_const_transitions_for_group(*group_it).size();
    }
    return total;
}

int TransitionSystem::unique_unlabeled_transitions() const {
    vector<Transition> unique_transitions;
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &trans = get_const_transitions_for_group(*group_it);
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

void TransitionSystem::dump_dot_graph() const {
    assert(is_valid());
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
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &trans = get_const_transitions_for_group(*group_it);
        for (size_t i = 0; i < trans.size(); ++i) {
            int src = trans[i].src;
            int target = trans[i].target;
            cout << "    node" << src << " -> node" << target << " [labels = ";
            for (LabelConstIter label_it = group_it->begin();
                 label_it != group_it->end(); ++label_it) {
                if (label_it != group_it->begin())
                    cout << ",";
                cout << "l" << *label_it;
            }
            cout << "];" << endl;
        }
    }
    cout << "}" << endl;
}

void TransitionSystem::dump_labels_and_transitions() const {
    cout << tag() << "transitions" << endl;
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        cout << "labels: ";
        for (LabelConstIter label_it = group_it->begin();
             label_it != group_it->end(); ++label_it) {
            if (label_it != group_it->begin())
                cout << ",";
            cout << *label_it << " (represented by " << get<0>(label_to_positions[*label_it]) << ")";
        }
        cout << endl;
        cout << "transitions: ";
        const vector<Transition> &trans = get_const_transitions_for_group(*group_it);
        for (size_t i = 0; i < trans.size(); ++i) {
            int src = trans[i].src;
            int target = trans[i].target;
            if (i != 0)
                cout << ",";
            cout << src << " -> " << target;
        }
        cout << endl;
    }
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

    /*
      Prepare grouped_labels data structure: add one single-element
      list for every operator.
    */
    for (int label_no = 0; label_no < static_cast<int>(g_operators.size()); ++label_no) {
        LabelGroupIter group_it = grouped_labels.insert(grouped_labels.end(), list<int>());
        LabelIter label_it = group_it->insert(group_it->end(), label_no);
        assert(*label_it == label_no);
        label_to_positions[label_no] = make_tuple(label_no, group_it, label_it);
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

    int multiplier = ts2_size;
    for (LabelGroupConstIter group1_it = ts1->grouped_labels.begin();
         group1_it != ts1->grouped_labels.end(); ++group1_it) {
        // Distribute the labels of this group among the "buckets"
        // corresponding to the groups of ts2.
        unordered_map<int, vector<int> > buckets;
        for (LabelConstIter label_it = group1_it->begin();
             label_it != group1_it->end(); ++label_it) {
            int label_no = *label_it;
            // TODO: hash via index
            int group2_transitions_index = get<0>(ts2->label_to_positions[label_no]);
            buckets[group2_transitions_index].push_back(label_no);
        }
        // Now buckets contains all equivalence classes that are
        // refinements of group1.

        // Now create the new groups together with their transitions.;
        const vector<Transition> &transitions1 = ts1->get_const_transitions_for_group(*group1_it);
        vector<int> dead_labels;
        for (const auto &bucket : buckets) {
            const vector<Transition> &transitions2 =
                ts2->transitions_by_group_index[bucket.first];

            // Create the new transitions for this bucket
            vector<Transition> new_transitions;
            // TODO: test against overflow? pitfall: transitions could be empty!
            if (transitions1.size() * transitions2.size() > new_transitions.max_size())
                exit_with(EXIT_OUT_OF_MEMORY);
            new_transitions.reserve(transitions1.size() * transitions2.size());
            for (size_t i = 0; i < transitions1.size(); ++i) {
                int src1 = transitions1[i].src;
                int target1 = transitions1[i].target;
                for (size_t j = 0; j < transitions2.size(); ++j) {
                    int src2 = transitions2[j].src;
                    int target2 = transitions2[j].target;
                    int src = src1 * multiplier + src2;
                    int target = target1 * multiplier + target2;
                    new_transitions.push_back(Transition(src, target));
                }
            }

            // Create a new group if the transitions are not empty
            const vector<int> &new_labels = bucket.second;
            if (new_transitions.empty()) {
                dead_labels.insert(dead_labels.end(), new_labels.begin(), new_labels.end());
            } else {
                sort(new_transitions.begin(), new_transitions.end());
                assert(is_sorted_unique(new_transitions));
                LabelGroupIter group_it = grouped_labels.insert(grouped_labels.end(), list<int>());
                int new_transitions_index = new_labels[0];
                for (size_t i = 0; i < new_labels.size(); ++i) {
                    int label_no = new_labels[i];
                    LabelIter label_it = group_it->insert(group_it->end(), label_no);
                    label_to_positions[label_no] = make_tuple(new_transitions_index, group_it, label_it);
                }
                transitions_by_group_index[new_transitions_index].swap(new_transitions);
            }
        }
        if (!dead_labels.empty()) {
            // TODO: replicated code from just above.
            LabelGroupIter group_it = grouped_labels.insert(grouped_labels.end(), list<int>());
            int new_transitions_index = dead_labels[0];
            for (size_t i = 0; i < dead_labels.size(); ++i) {
                int label_no = dead_labels[i];
                LabelIter label_it = group_it->insert(group_it->end(), label_no);
                label_to_positions[label_no] = make_tuple(new_transitions_index, group_it, label_it);
            }
            // No need to swap empty transitions
        }
    }

    assert(are_transitions_sorted_unique());
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
