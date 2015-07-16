#include "transition_system.h"

#include "labels.h"

#include "../priority_queue.h"
#include "../task_proxy.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <ctype.h>
#include <deque>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace std;

/* Implementation note: Transitions are grouped by their label groups,
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


TransitionSystem::TransitionSystem(const TaskProxy &task_proxy,
                                   const Labels *labels)
    : labels(labels),
      num_labels(labels->get_size()),
      num_variables(task_proxy.get_variables().size()) {
    clear_distances();
    size_t num_ops = task_proxy.get_operators().size();
    if (num_ops > 0) {
        size_t max_num_labels = num_ops * 2 - 1;
        transitions_of_groups.resize(max_num_labels);
        label_to_positions.resize(max_num_labels);
    }
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
        const vector<Transition> &transitions = group_it->get_const_transitions();
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &transition = transitions[j];
            forward_graph[transition.src].push_back(transition.target);
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
        const vector<Transition> &transitions = group_it->get_const_transitions();
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &transition = transitions[j];
            backward_graph[transition.target].push_back(transition.src);
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
        const vector<Transition> &transitions = group_it->get_const_transitions();
        int cost = group_it->get_cost();
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &transition = transitions[j];
            forward_graph[transition.src].push_back(
                make_pair(transition.target, cost));
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
        const vector<Transition> &transitions = group_it->get_const_transitions();
        int cost = group_it->get_cost();
        for (size_t j = 0; j < transitions.size(); ++j) {
            const Transition &transition = transitions[j];
            backward_graph[transition.target].push_back(
                make_pair(transition.src, cost));
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

void TransitionSystem::normalize_given_transitions(vector<Transition> &transitions) const {
    sort(transitions.begin(), transitions.end());
    transitions.erase(unique(transitions.begin(), transitions.end()), transitions.end());
}

bool TransitionSystem::are_transitions_sorted_unique() const {
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = group_it->get_const_transitions();
        if (!is_sorted_unique(transitions)) {
            return false;
        }
    }
    return true;
}

bool TransitionSystem::is_label_reduced() const {
    return num_labels == labels->get_size();
}

void TransitionSystem::add_label_to_group(LabelGroupIter group_it,
                                          int label_no,
                                          bool update_cost) {
    LabelIter label_it = group_it->insert(label_no);
    label_to_positions[label_no] = make_tuple(group_it, label_it);
    if (update_cost) {
        int label_cost = labels->get_label_cost(label_no);
        if (label_cost < group_it->get_cost()) {
            group_it->set_cost(label_cost);
        }
    }
}

int TransitionSystem::add_label_group(const vector<int> &new_labels) {
    int new_index = new_labels[0];
    LabelGroupIter group_it = add_empty_label_group(&transitions_of_groups[new_index]);
    for (size_t i = 0; i < new_labels.size(); ++i) {
        int label_no = new_labels[i];
        add_label_to_group(group_it, label_no);
    }
    return new_index;
}

void TransitionSystem::compute_locally_equivalent_labels() {
    /*
      Compare every group of labels and their transitions to all others and
      merge two groups whenever the transitions are the same.
    */
    for (LabelGroupIter group1_it = grouped_labels.begin();
         group1_it != grouped_labels.end(); ++group1_it) {
        const vector<Transition> &transitions1 = group1_it->get_const_transitions();
        for (LabelGroupIter group2_it = group1_it;
             group2_it != grouped_labels.end(); ++group2_it) {
            if (group2_it == group1_it)
                continue;
            vector<Transition> &transitions2 = group2_it->get_transitions();
            if ((transitions1.empty() && transitions2.empty()) || transitions1 == transitions2) {
                for (LabelConstIter group2_label_it = group2_it->begin();
                     group2_label_it != group2_it->end(); ++group2_label_it) {
                    int other_label_no = *group2_label_it;
                    add_label_to_group(group1_it, other_label_no);
                }
                grouped_labels.erase(group2_it);
                --group2_it;
                release_vector_memory(transitions2);
            }
        }
    }
}

void TransitionSystem::build_atomic_transition_systems(const TaskProxy &task_proxy,
                                                       vector<TransitionSystem *> &result,
                                                       Labels *labels) {
    assert(result.empty());
    cout << "Building atomic transition systems... " << endl;
    VariablesProxy variables = task_proxy.get_variables();
    OperatorsProxy operators = task_proxy.get_operators();

    // Step 1: Create the transition system objects without transitions.
    for (VariableProxy var : variables)
        result.push_back(new AtomicTransitionSystem(task_proxy, labels, var.get_id()));

    // Step 2: Add transitions.
    vector<vector<bool> > relevant_labels(result.size(), vector<bool>(operators.size(), false));
    for (OperatorProxy op : operators) {
        int label_no = op.get_id();
        labels->add_label(op.get_cost());
        PreconditionsProxy preconditions = op.get_preconditions();
        EffectsProxy effects = op.get_effects();
        unordered_map<int, int> pre_val;
        vector<bool> has_effect_on_var(variables.size(), false);
        for (FactProxy precondition : preconditions)
            pre_val[precondition.get_variable().get_id()] = precondition.get_value();

        for (EffectProxy effect : effects) {
            FactProxy fact = effect.get_fact();
            VariableProxy var = fact.get_variable();
            int var_id = var.get_id();
            has_effect_on_var[var_id] = true;
            int post_value = fact.get_value();
            TransitionSystem *ts = result[var_id];

            // Determine possible values that var can have when this
            // operator is applicable.
            int pre_value = -1;
            auto pre_val_it = pre_val.find(var_id);
            if (pre_val_it != pre_val.end())
                pre_value = pre_val_it->second;
            int pre_value_min, pre_value_max;
            if (pre_value == -1) {
                pre_value_min = 0;
                pre_value_max = var.get_domain_size();
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
            EffectConditionsProxy effect_conditions = effect.get_conditions();
            int cond_effect_pre_value = -1;
            bool has_other_effect_cond = false;
            for (FactProxy condition : effect_conditions) {
                if (condition.get_variable() == var) {
                    cond_effect_pre_value = condition.get_value();
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
                    ts->transitions_of_groups[label_no].push_back(trans);
                }
            }

            // Handle transitions that occur when the effect does not trigger.
            if (!effect_conditions.empty()) {
                for (int value = pre_value_min; value < pre_value_max; ++value) {
                    /* Add self-loop if the effect might not trigger.
                       If the effect has a condition on another variable, then
                       it can fail to trigger no matter which value var has.
                       If it only has a condition on var, then the effect
                       fails to trigger if this condition is false. */
                    if (has_other_effect_cond || value != cond_effect_pre_value) {
                        Transition loop(value, value);
                        ts->transitions_of_groups[label_no].push_back(loop);
                    }
                }
            }

            relevant_labels[var_id][label_no] = true;
        }

        for (FactProxy precondition : preconditions) {
            int var_id = precondition.get_variable().get_id();
            if (!has_effect_on_var[var_id]) {
                int value = precondition.get_value();
                TransitionSystem *ts = result[var_id];
                Transition trans(value, value);
                ts->transitions_of_groups[label_no].push_back(trans);
                relevant_labels[var_id][label_no] = true;
            }
        }
    }

    for (size_t i = 0; i < result.size(); ++i) {
        // Need to set the correct number of labels *after* generating them
        TransitionSystem *ts = result[i];
        ts->num_labels = labels->get_size();
        /* Make all irrelevant labels explicit and set the cost of every
           singleton label group. */
        for (int label_no = 0; label_no < ts->num_labels; ++label_no) {
            if (!relevant_labels[i][label_no]) {
                for (int state = 0; state < ts->num_states; ++state) {
                    Transition loop(state, state);
                    ts->transitions_of_groups[label_no].push_back(loop);
                }
            }
            ts->get_group_it(label_no)->set_cost(labels->get_label_cost(label_no));
        }
        ts->compute_locally_equivalent_labels();
        ts->compute_distances_and_prune();
        assert(ts->is_valid());
    }
}

bool TransitionSystem::apply_abstraction(
    const vector<forward_list<AbstractStateRef> > &collapsed_groups) {
    assert(is_valid());

    if (static_cast<int>(collapsed_groups.size()) == get_size()) {
        cout << tag() << "not applying abstraction (same number of states)" << endl;
        return false;
    }

    cout << tag() << "applying abstraction (" << get_size()
         << " to " << collapsed_groups.size() << " states)" << endl;

    typedef forward_list<AbstractStateRef> Group;

    vector<int> abstraction_mapping(num_states, PRUNED_STATE);

    for (size_t group_no = 0; group_no < collapsed_groups.size(); ++group_no) {
        const Group &group = collapsed_groups[group_no];
        for (Group::const_iterator pos = group.begin(); pos != group.end(); ++pos) {
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
        const Group &group = collapsed_groups[new_state];
        assert(!group.empty());

        Group::const_iterator pos = group.begin();
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
    release_vector_memory(init_distances);
    release_vector_memory(goal_distances);
    release_vector_memory(goal_states);

    // Update all transitions. Locally equivalent labels remain locally equivalent.
    for (LabelGroupIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        vector<Transition> &transitions = group_it->get_transitions();
        vector<Transition> new_transitions;
        for (size_t i = 0; i < transitions.size(); ++i) {
            const Transition &transition = transitions[i];
            int src = abstraction_mapping[transition.src];
            int target = abstraction_mapping[transition.target];
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
    return true;
}

void TransitionSystem::apply_label_reduction(const vector<pair<int, vector<int> > > &label_mapping,
                                             bool only_equivalent_labels) {
    assert(!is_label_reduced());
    assert(are_distances_computed());
    assert(are_transitions_sorted_unique());

    /*
      We iterate over the given label mapping, treating every new label and
      the reduced old labels separately. We further distinguish the case
      where we know that the reduced labels are all from the same equivalence
      group from the case where we may combine arbitrary labels. We also
      assume that only labels of the same cost are reduced.

      The case where only equivalent labels are combined is simple: remove all
      old labels from the label group and add the new one.

      The other case is more involved: again remove all old labels from their
      groups, and the groups themselves if they become empty. Also collect
      the transitions of all reduced labels. Add a new group for every new
      label and assign the collected transitions to this group. Recompute the
      cost of all groups and compute locally equivalent labels.

      NOTE: Previously, this latter case was computed in a more incremental
      fashion: Rather than recomputing cost of all groups, we only recomputed
      cost for groups from which we actually removed labels (hence temporarily
      storing these affected groups). Furthermore, rather than computing
      locally equivalent labels from scratch, we did not per default add a new
      group for every label, but checked for an existing equivalent label
      group. In issue539, it turned out that this incremental fashion of
      computation does not accelerate the computation.
    */

    for (size_t i = 0; i < label_mapping.size(); ++i) {
        const vector<int> &old_label_nos = label_mapping[i].second;
        assert(old_label_nos.size() >= 2);
        int new_label_no = label_mapping[i].first;
        assert(new_label_no == num_labels);

        // Assert that all old labels are in the correct range
        for (size_t j = 0; j < old_label_nos.size(); ++j) {
            assert(old_label_nos[j] < num_labels);
        }

        /*
          Remove all existing labels from their group(s) and possibly the
          groups themselves.
        */
        LabelGroupIter canonical_group_it = get_group_it(old_label_nos[0]);
        // We use a set to collect the reduced label's transitions so that they are sorted.
        set<Transition> collected_transitions;
        for (size_t i = 0; i < old_label_nos.size(); ++i) {
            int label_no = old_label_nos[i];
            LabelGroupIter group_it = get_group_it(label_no);
            if (only_equivalent_labels) {
                assert(group_it == canonical_group_it);
            } else {
                const vector<Transition> &old_transitions =
                    group_it->get_const_transitions();
                collected_transitions.insert(old_transitions.begin(), old_transitions.end());
            }
            LabelIter label_it = get_label_it(label_no);
            group_it->erase(label_it);
            // Note: we cannot invalidate the tupel label_to_positions[label_no]
            if (!only_equivalent_labels) {
                if (group_it->empty()) {
                    release_vector_memory(group_it->get_transitions());
                    grouped_labels.erase(group_it);
                }
            }
        }

        if (only_equivalent_labels) {
            // No need to update the group's cost due to the assumption of exact label reduction.
            add_label_to_group(canonical_group_it, new_label_no, false);
        } else {
            transitions_of_groups[new_label_no].assign(
                collected_transitions.begin(), collected_transitions.end());
            LabelGroupIter group_it =
                add_empty_label_group(&transitions_of_groups[new_label_no]);
            add_label_to_group(group_it, new_label_no);
        }
        ++num_labels;
    }

    if (!only_equivalent_labels) {
        // Recompute the cost all label groups.
        for (LabelGroupIter group_it = grouped_labels.begin();
             group_it != grouped_labels.end(); ++group_it) {
            group_it->set_cost(INF);
            for (LabelConstIter label_it = group_it->begin();
                 label_it != group_it->end(); ++label_it) {
                int cost = labels->get_label_cost(*label_it);
                if (cost < group_it->get_cost()) {
                    group_it->set_cost(cost);
                }
            }
        }
        compute_locally_equivalent_labels();
    }

    assert(is_valid());
}

void TransitionSystem::release_memory() {
    list<LabelGroup>().swap(grouped_labels);
    release_vector_memory(transitions_of_groups);
    release_vector_memory(label_to_positions);
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

int TransitionSystem::get_cost(const State &state) const {
    assert(are_distances_computed());
    int abs_state = get_abstract_state(state);
    if (abs_state == PRUNED_STATE)
        return -1;
    int cost = goal_distances[abs_state];
    assert(cost != INF);
    return cost;
}

int TransitionSystem::total_transitions() const {
    int total = 0;
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        total += group_it->get_const_transitions().size();
    }
    return total;
}

int TransitionSystem::unique_unlabeled_transitions() const {
    vector<Transition> unique_transitions;
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        const vector<Transition> &transitions = group_it->get_const_transitions();
        unique_transitions.insert(unique_transitions.end(), transitions.begin(),
                                  transitions.end());
    }
    ::sort(unique_transitions.begin(), unique_transitions.end());
    return unique(unique_transitions.begin(), unique_transitions.end())
           - unique_transitions.begin();
}

void TransitionSystem::statistics(const Timer &timer,
                                  bool include_expensive_statistics) const {
    cout << tag() << get_size() << " states, ";
    if (include_expensive_statistics)
        cout << unique_unlabeled_transitions();
    else
        cout << "???";
    cout << "/" << total_transitions() << " arcs, " << endl;
    cout << tag();
    if (!are_distances_computed()) {
        cout << "distances not computed";
    } else if (is_solvable()) {
        cout << "init h=" << goal_distances[init_state] << ", max f=" << max_f
             << ", max g=" << max_g << ", max h=" << max_h;
    } else {
        cout << "transition system is unsolvable";
    }
    cout << " [t=" << timer << "]" << endl;
}

void TransitionSystem::dump_dot_graph() const {
    assert(is_valid());
    cout << "digraph transition system";
    for (size_t i = 0; i < var_id_set.size(); ++i)
        cout << "_" << var_id_set[i];
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
        const vector<Transition> &transitions = group_it->get_const_transitions();
        for (size_t i = 0; i < transitions.size(); ++i) {
            int src = transitions[i].src;
            int target = transitions[i].target;
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
            cout << *label_it;
        }
        cout << endl;
        cout << "transitions: ";
        const vector<Transition> &transitions = group_it->get_const_transitions();
        for (size_t i = 0; i < transitions.size(); ++i) {
            int src = transitions[i].src;
            int target = transitions[i].target;
            if (i != 0)
                cout << ",";
            cout << src << " -> " << target;
        }
        cout << endl;
        cout << "cost: " << group_it->get_cost() << endl;
    }
}



AtomicTransitionSystem::AtomicTransitionSystem(const TaskProxy &task_proxy,
                                               const Labels *labels,
                                               int var_id)
    : TransitionSystem(task_proxy, labels), var_id(var_id) {
    var_id_set.push_back(var_id);
    /*
      This generates the states of the atomic transition system, but not the
      arcs: It is more efficient to generate all arcs of all atomic
      transition systems simultaneously.
     */
    int range = task_proxy.get_variables()[var_id].get_domain_size();

    int init_value = task_proxy.get_initial_state()[var_id].get_value();
    int goal_value = -1;
    goal_relevant = false;
    GoalsProxy goals = task_proxy.get_goals();
    for (FactProxy goal : goals) {
        if (goal.get_variable().get_id() == var_id) {
            goal_relevant = true;
            assert(goal_value == -1);
            goal_value = goal.get_value();
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
      group for every operator.
    */
    int num_ops = task_proxy.get_operators().size();
    for (int label_no = 0; label_no < num_ops; ++label_no) {
        // We use the label number as index for transitions of groups
        LabelGroupIter group_it = add_empty_label_group(&transitions_of_groups[label_no]);
        add_label_to_group(group_it, label_no, false);
        /* We cannot set the cost here, because the labels have not been
           created yet. */
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
    s << "atomic transition system #" << var_id;
    return s.str();
}

AbstractStateRef AtomicTransitionSystem::get_abstract_state(const State &state) const {
    int value = state[var_id].get_value();
    return lookup_table[value];
}



CompositeTransitionSystem::CompositeTransitionSystem(const TaskProxy &task_proxy,
                                                     const Labels *labels,
                                                     TransitionSystem *ts1,
                                                     TransitionSystem *ts2)
    : TransitionSystem(task_proxy, labels) {
    cout << "Merging " << ts1->description() << " and "
         << ts2->description() << endl;

    assert(ts1->is_solvable() && ts2->is_solvable());
    assert(ts1->is_valid() && ts2->is_valid());

    components[0] = ts1;
    components[1] = ts2;

    ::set_union(ts1->var_id_set.begin(), ts1->var_id_set.end(), ts2->var_id_set.begin(),
                ts2->var_id_set.end(), back_inserter(var_id_set));

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

    /*
      We can compute the local equivalence relation of a composite T
      from the local equivalence relations of the two components T1 and T2:
      l and l' are locally equivalent in T iff:
      (A) they are local equivalent in T1 and in T2, or
      (B) they are both dead in T (e.g., this includes the case where
          l is dead in T1 only and l' is dead in T2 only, so they are not
          locally equivalent in either of the components).
    */
    int multiplier = ts2_size;
    vector<int> dead_labels;
    for (LabelGroupConstIter group1_it = ts1->grouped_labels.begin();
         group1_it != ts1->grouped_labels.end(); ++group1_it) {
        // Distribute the labels of this group among the "buckets"
        // corresponding to the groups of ts2.
        unordered_map<const LabelGroup *, vector<int> > buckets;
        for (LabelConstIter label_it = group1_it->begin();
             label_it != group1_it->end(); ++label_it) {
            int label_no = *label_it;
            LabelGroupIter group_it = ts2->get_group_it(label_no);
            buckets[&*group_it].push_back(label_no);
        }
        // Now buckets contains all equivalence classes that are
        // refinements of group1.

        // Now create the new groups together with their transitions.
        const vector<Transition> &transitions1 = group1_it->get_const_transitions();
        for (const auto &bucket : buckets) {
            const vector<Transition> &transitions2 = bucket.first->get_const_transitions();

            // Create the new transitions for this bucket
            vector<Transition> new_transitions;
            if (transitions1.size() && transitions2.size()
                && transitions1.size() > new_transitions.max_size() / transitions2.size())
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
                int new_index = add_label_group(new_labels);
                transitions_of_groups[new_index].swap(new_transitions);
            }
        }
    }

    /*
      We collect all dead labels separately, because the bucket refining
      does not work in cases where there are at least two dead labels l1
      and l2 in the composite, where l1 was only a dead label in the first
      component and l2 was only a dead label in the second component.
      All dead labels should form one single label group.
    */
    if (!dead_labels.empty()) {
        // Dead labels have empty transitions
        add_label_group(dead_labels);
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
    s << "transition system (" << var_id_set.size() << "/"
      << num_variables << " vars)";
    return s.str();
}

AbstractStateRef CompositeTransitionSystem::get_abstract_state(const State &state) const {
    AbstractStateRef state1 = components[0]->get_abstract_state(state);
    AbstractStateRef state2 = components[1]->get_abstract_state(state);
    if (state1 == PRUNED_STATE || state2 == PRUNED_STATE)
        return PRUNED_STATE;
    return lookup_table[state1][state2];
}
