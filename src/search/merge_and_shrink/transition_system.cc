#include "transition_system.h"

#include "distances.h"
#include "heuristic_representation.h"
#include "label_equivalence_relation.h"
#include "labels.h"

#include "../task_proxy.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace std;

/*
  Implementation note: Transitions are grouped by their label groups,
  not by source state or any such thing. Such a grouping is beneficial
  for fast generation of products because we can iterate label group
  by label group, and it also allows applying transition system
  mappings very efficiently.

  We rarely need to be able to efficiently query the successors of a
  given state; actually, only the distance computation requires that,
  and it simply generates such a graph representation of the
  transitions itself. Various experiments have shown that maintaining
  a graph representation permanently for the benefit of distance
  computation is not worth the overhead.
*/

const int INF = numeric_limits<int>::max();


const int TransitionSystem::PRUNED_STATE;


TSConstIterator::TSConstIterator(
    const shared_ptr<LabelEquivalenceRelation> label_equivalence_relation,
    const vector<vector<Transition> > &transitions_by_group_id,
    bool end)
    : label_equivalence_relation(label_equivalence_relation),
      transitions_by_group_id(transitions_by_group_id),
      current((end ? label_equivalence_relation->get_size() : 0)) {
    while (current < label_equivalence_relation->get_size()
           && (*label_equivalence_relation)[current].empty()) {
        ++current;
    }
}

void TSConstIterator::operator++() {
    ++current;
    while (current < label_equivalence_relation->get_size()
           && (*label_equivalence_relation)[current].empty()) {
        ++current;
    }
}

int TSConstIterator::get_cost() const {
    return (*label_equivalence_relation)[current].get_cost();
}

LabelConstIter TSConstIterator::begin() const {
    return (*label_equivalence_relation)[current].begin();
}

LabelConstIter TSConstIterator::end() const {
    return (*label_equivalence_relation)[current].end();
}


// common case for both constructors
TransitionSystem::TransitionSystem(const TaskProxy &task_proxy,
                                   const shared_ptr<Labels> labels)
    : num_variables(task_proxy.get_variables().size()),
      label_equivalence_relation(make_shared<LabelEquivalenceRelation>(labels)),
      heuristic_representation(nullptr),
      distances(make_unique_ptr<Distances>(*this)) {
    transitions_by_group_id.resize(labels->get_max_size());
}

// atomic transition system constructor
TransitionSystem::TransitionSystem(
    const TaskProxy &task_proxy,
    const shared_ptr<Labels> labels,
    int var_id,
    vector<vector<Transition> > && transitions_by_label)
    : TransitionSystem(task_proxy, labels) {
    /*
      TODO: Once we no longer delegate to another constructor,
      the following line can be changed to an initialization:
      ": transitions_of_groups(transitions_by_label)"
    */
    transitions_by_group_id = move(transitions_by_label);
    /*
      TODO: The following if block and the interaction with the
      constructor we delegate to are a hack and a bit of a performance
      concern. The base constructor resizes transitions_of_groups to
      size 2 * num_ops - 1, but then we discard it to move the
      passed-in transitions into the object instead. Then we resize it
      to size 2 * num_ops - 1 again.

      So there are two unnecessary resizes, which of course don't come
      for free. A potential fix would be requiring the factory to
      already create transitions_of_groups with the appropriate size,
      but this would perhaps leak an implementation detail that the
      factory should not care about. For now, let's leave
    */
    int num_ops = task_proxy.get_operators().size();
    if (num_ops > 0) {
        int max_num_labels = num_ops * 2 - 1;
        transitions_by_group_id.resize(max_num_labels);
    }

    incorporated_variables.push_back(var_id);

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
    goal_states.resize(num_states, false);
    for (int value = 0; value < range; ++value) {
        if (value == goal_value || goal_value == -1) {
            goal_states[value] = true;
        }
        if (value == init_value)
            init_state = value;
    }

    heuristic_representation = make_unique_ptr<HeuristicRepresentationLeaf>(
        var_id, range);

    /*
      Prepare label_equivalence_relation data structure: add one single-element
      group for every operator.
    */
    for (int label_no = 0; label_no < num_ops; ++label_no) {
        // We use the label number as index for transitions of groups
        label_equivalence_relation->add_label_group({label_no}
                                                    );
        // We could assert that the return value equals label_no, but not
        // easily in release mode without unused variable error.
    }

    compute_locally_equivalent_labels();
    compute_distances_and_prune();
    assert(is_valid());
}

// constructor for merges
TransitionSystem::TransitionSystem(const TaskProxy &task_proxy,
                                   const shared_ptr<Labels> labels,
                                   TransitionSystem *ts1,
                                   TransitionSystem *ts2)
    : TransitionSystem(task_proxy, labels) {
    cout << "Merging " << ts1->description() << " and "
         << ts2->description() << endl;

    assert(ts1->is_solvable() && ts2->is_solvable());
    assert(ts1->is_valid() && ts2->is_valid());

    ::set_union(
        ts1->incorporated_variables.begin(), ts1->incorporated_variables.end(),
        ts2->incorporated_variables.begin(), ts2->incorporated_variables.end(),
        back_inserter(incorporated_variables));

    int ts1_size = ts1->get_size();
    int ts2_size = ts2->get_size();
    num_states = ts1_size * ts2_size;
    goal_states.resize(num_states, false);
    goal_relevant = (ts1->goal_relevant || ts2->goal_relevant);

    for (int s1 = 0; s1 < ts1_size; ++s1) {
        for (int s2 = 0; s2 < ts2_size; ++s2) {
            int state = s1 * ts2_size + s2;
            if (ts1->goal_states[s1] && ts2->goal_states[s2])
                goal_states[state] = true;
            if (s1 == ts1->init_state && s2 == ts2->init_state)
                init_state = state;
        }
    }

    heuristic_representation = make_unique_ptr<HeuristicRepresentationMerge>(
        move(ts1->heuristic_representation),
        move(ts2->heuristic_representation));

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
    for (TSConstIterator group1_it = ts1->begin();
         group1_it != ts1->end(); ++group1_it) {
        // Distribute the labels of this group among the "buckets"
        // corresponding to the groups of ts2.
        unordered_map<int, vector<int> > buckets;
        for (LabelConstIter label_it = group1_it.begin();
             label_it != group1_it.end(); ++label_it) {
            int label_no = *label_it;
            int group2_id = ts2->label_equivalence_relation->get_group_id(label_no);
            buckets[group2_id].push_back(label_no);
        }
        // Now buckets contains all equivalence classes that are
        // refinements of group1.

        // Now create the new groups together with their transitions.
        const vector<Transition> &transitions1 = group1_it.get_transitions();
        for (const auto &bucket : buckets) {
            const vector<Transition> &transitions2 =
                ts2->get_transitions_for_group_id(bucket.first);

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
                int new_index = label_equivalence_relation->add_label_group(new_labels);
                transitions_by_group_id[new_index].swap(new_transitions);
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
        label_equivalence_relation->add_label_group(dead_labels);
    }

    assert(are_transitions_sorted_unique());
    compute_distances_and_prune();
    assert(is_valid());
}

TransitionSystem::~TransitionSystem() {
}

bool TransitionSystem::is_valid() const {
    return distances->are_distances_computed()
           && are_transitions_sorted_unique();
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

void TransitionSystem::compute_distances_and_prune() {
    /*
      This method does all that compute_distances does and
      additionally prunes all states that are unreachable (abstract g
      is infinite) or irrelevant (abstract h is infinite).
    */
    assert(are_transitions_sorted_unique());
    discard_states(distances->compute_distances());
}

void TransitionSystem::normalize_given_transitions(vector<Transition> &transitions) const {
    sort(transitions.begin(), transitions.end());
    transitions.erase(unique(transitions.begin(), transitions.end()), transitions.end());
}

bool TransitionSystem::are_transitions_sorted_unique() const {
    for (TSConstIterator group_it = begin();
         group_it != end(); ++group_it) {
        if (!is_sorted_unique(group_it.get_transitions()))
            return false;
    }
    return true;
}

void TransitionSystem::compute_locally_equivalent_labels() {
    /*
      Compare every group of labels and their transitions to all others and
      merge two groups whenever the transitions are the same.
    */
    for (TSConstIterator group1_it = begin();
         group1_it != end(); ++group1_it) {
        const vector<Transition> &transitions1 = group1_it.get_transitions();
        for (TSConstIterator group2_it = group1_it;
             group2_it != end(); ++group2_it) {
            if (group2_it != group1_it) {
                int group2_id = group2_it.get_id();
                vector<Transition> &transitions2 = get_transitions_for_group_id(group2_id);
                if ((transitions1.empty() && transitions2.empty()) || transitions1 == transitions2) {
                    label_equivalence_relation->move_group_into_group(
                        group2_id, group1_it.get_id());
                    release_vector_memory(transitions2);
                }
            }
        }
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
    vector<bool> new_goal_states(new_num_states, false);

    for (AbstractStateRef new_state = 0; new_state < new_num_states; ++new_state) {
        const Group &group = collapsed_groups[new_state];
        assert(!group.empty());

        Group::const_iterator pos = group.begin();
        new_goal_states[new_state] = goal_states[*pos];

        ++pos;
        for (; pos != group.end(); ++pos)
            if (goal_states[*pos])
                new_goal_states[new_state] = true;
    }

    goal_states = move(new_goal_states);

    // Update all transitions. Locally equivalent labels remain locally equivalent.
    for (TSConstIterator group_it = begin();
         group_it != end(); ++group_it) {
        vector<Transition> &transitions = get_transitions_for_group_id(group_it.get_id());
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
    init_state = abstraction_mapping[init_state];
    if (init_state == PRUNED_STATE)
        cout << tag() << "initial state pruned; task unsolvable" << endl;

    if (!distances->apply_abstraction(collapsed_groups))
        cout << tag() << "simplification was not f-preserving!" << endl;
    heuristic_representation->apply_abstraction_to_lookup_table(
        abstraction_mapping);

    assert(is_valid());
    return true;
}

void TransitionSystem::apply_label_reduction(const vector<pair<int, vector<int> > > &label_mapping,
                                             bool only_equivalent_labels) {
    assert(distances->are_distances_computed());
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

        if (only_equivalent_labels) {
            label_equivalence_relation->replace_labels_by_label(old_label_nos, new_label_no);
        } else {
            /*
              Remove all existing labels from their group(s) and possibly the
              groups themselves.
            */
            // We use a set to collect the reduced label's transitions so that they are sorted.
            set<Transition> collected_transitions;
            for (int old_label_no : old_label_nos) {
                int group_id = label_equivalence_relation->get_group_id(old_label_no);
                vector<Transition> &old_transitions =
                    get_transitions_for_group_id(group_id);
                collected_transitions.insert(old_transitions.begin(), old_transitions.end());
                bool remove_group = label_equivalence_relation->erase(old_label_no);
                if (remove_group) {
                    release_vector_memory(old_transitions);
                }
            }
            int group_id = label_equivalence_relation->add_label_group({new_label_no}
                                                                       );
            transitions_by_group_id[group_id].assign(
                collected_transitions.begin(), collected_transitions.end());
        }
    }

    if (!only_equivalent_labels) {
        // Recompute the cost of all label groups.
        label_equivalence_relation->recompute_group_cost();
        compute_locally_equivalent_labels();
    }

    assert(is_valid());
}

void TransitionSystem::release_memory() {
    label_equivalence_relation.reset();
    release_vector_memory(transitions_by_group_id);
}

string TransitionSystem::tag() const {
    string desc(description());
    desc[0] = toupper(desc[0]);
    return desc + ": ";
}

bool TransitionSystem::is_solvable() const {
    assert(distances->are_distances_computed());
    return init_state != PRUNED_STATE;
}

int TransitionSystem::get_cost(const State &state) const {
    assert(distances->are_distances_computed());
    int abs_state = heuristic_representation->get_abstract_state(state);

    if (abs_state == PRUNED_STATE)
        return -1;
    int cost = distances->get_goal_distance(abs_state);
    assert(cost != INF);
    return cost;
}

int TransitionSystem::total_transitions() const {
    int total = 0;
    for (TSConstIterator group_it = begin();
         group_it != end(); ++group_it) {
        total += group_it.get_transitions().size();
    }
    return total;
}

int TransitionSystem::unique_unlabeled_transitions() const {
    vector<Transition> unique_transitions;
    for (TSConstIterator group_it = begin();
         group_it != end(); ++group_it) {
        const vector<Transition> &transitions = group_it.get_transitions();
        unique_transitions.insert(unique_transitions.end(), transitions.begin(),
                                  transitions.end());
    }
    ::sort(unique_transitions.begin(), unique_transitions.end());
    return unique(unique_transitions.begin(), unique_transitions.end())
           - unique_transitions.begin();
}

string TransitionSystem::description() const {
    ostringstream s;
    if (incorporated_variables.size() == 1) {
        s << "atomic transition system #" << *incorporated_variables.begin();
    } else {
        s << "composite transition system with "
          << incorporated_variables.size() << "/" << num_variables << " vars";
    }
    return s.str();
}

void TransitionSystem::statistics(const Timer &timer) const {
    cout << tag() << get_size() << " states, "
         << total_transitions() << " arcs " << endl;
    // TODO: Turn the following block into Distances::statistics()?
    cout << tag();
    if (!distances->are_distances_computed()) {
        cout << "distances not computed";
    } else if (is_solvable()) {
        cout << "init h=" << distances->get_goal_distance(init_state)
             << ", max f=" << distances->get_max_f()
             << ", max g=" << distances->get_max_g()
             << ", max h=" << distances->get_max_h();
    } else {
        cout << "transition system is unsolvable";
    }
    cout << " [t=" << timer << "]" << endl;
}

void TransitionSystem::dump_dot_graph() const {
    assert(is_valid());
    cout << "digraph transition_system";
    for (size_t i = 0; i < incorporated_variables.size(); ++i)
        cout << "_" << incorporated_variables[i];
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
    for (TSConstIterator group_it = begin();
         group_it != end(); ++group_it) {
        const vector<Transition> &transitions = group_it.get_transitions();
        for (size_t i = 0; i < transitions.size(); ++i) {
            int src = transitions[i].src;
            int target = transitions[i].target;
            cout << "    node" << src << " -> node" << target << " [labels = ";
            for (LabelConstIter label_it = group_it.begin();
                 label_it != group_it.end(); ++label_it) {
                if (label_it != group_it.begin())
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
    for (TSConstIterator group_it = begin();
         group_it != end(); ++group_it) {
        cout << "group id: " << group_it.get_id() << endl;
        cout << "labels: ";
        for (LabelConstIter label_it = group_it.begin();
             label_it != group_it.end(); ++label_it) {
            if (label_it != group_it.begin())
                cout << ",";
            cout << *label_it;
        }
        cout << endl;
        cout << "transitions: ";
        const vector<Transition> &transitions = group_it.get_transitions();
        for (size_t i = 0; i < transitions.size(); ++i) {
            int src = transitions[i].src;
            int target = transitions[i].target;
            if (i != 0)
                cout << ",";
            cout << src << " -> " << target;
        }
        cout << endl;
        cout << "cost: " << group_it.get_cost() << endl;
    }
}

int TransitionSystem::get_max_f() const {
    return distances->get_max_f();
}

int TransitionSystem::get_max_g() const {
    return distances->get_max_g();
}

int TransitionSystem::get_max_h() const {
    return distances->get_max_h();
}

int TransitionSystem::get_init_distance(int state) const {
    return distances->get_init_distance(state);
}

int TransitionSystem::get_goal_distance(int state) const {
    return distances->get_goal_distance(state);
}

int TransitionSystem::get_num_labels() const {
    return label_equivalence_relation->get_num_labels();
}
