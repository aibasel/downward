#include "transition_system.h"

#include "distances.h"
#include "label_equivalence_relation.h"
#include "labels.h"

#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;
using utils::ExitCode;

namespace merge_and_shrink {
ostream &operator<<(ostream &os, const Transition &trans) {
    os << trans.src << "->" << trans.target;
    return os;
}

// Sorts the given set of transitions and removes duplicates.
static void normalize_given_transitions(vector<Transition> &transitions) {
    sort(transitions.begin(), transitions.end());
    transitions.erase(unique(transitions.begin(), transitions.end()), transitions.end());
}

TSConstIterator::TSConstIterator(
    const LabelEquivalenceRelation &label_equivalence_relation,
    const vector<vector<Transition>> &transitions_by_group_id,
    bool end)
    : label_equivalence_relation(label_equivalence_relation),
      transitions_by_group_id(transitions_by_group_id),
      current_group_id((end ? label_equivalence_relation.get_size() : 0)) {
    next_valid_index();
}

void TSConstIterator::next_valid_index() {
    while (current_group_id < label_equivalence_relation.get_size()
           && label_equivalence_relation.is_empty_group(current_group_id)) {
        ++current_group_id;
    }
}

void TSConstIterator::operator++() {
    ++current_group_id;
    next_valid_index();
}

GroupAndTransitions TSConstIterator::operator*() const {
    return GroupAndTransitions(
        label_equivalence_relation.get_group(current_group_id),
        transitions_by_group_id[current_group_id]);
}


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

TransitionSystem::TransitionSystem(
    int num_variables,
    vector<int> &&incorporated_variables,
    unique_ptr<LabelEquivalenceRelation> &&label_equivalence_relation,
    vector<vector<Transition>> &&transitions_by_group_id,
    int num_states,
    vector<bool> &&goal_states,
    int init_state)
    : num_variables(num_variables),
      incorporated_variables(move(incorporated_variables)),
      label_equivalence_relation(move(label_equivalence_relation)),
      transitions_by_group_id(move(transitions_by_group_id)),
      num_states(num_states),
      goal_states(move(goal_states)),
      init_state(init_state) {
    assert(are_transitions_sorted_unique());
    assert(in_sync_with_label_equivalence_relation());
}

TransitionSystem::TransitionSystem(const TransitionSystem &other)
    : num_variables(other.num_variables),
      incorporated_variables(other.incorporated_variables),
      label_equivalence_relation(
          utils::make_unique_ptr<LabelEquivalenceRelation>(
              *other.label_equivalence_relation)),
      transitions_by_group_id(other.transitions_by_group_id),
      num_states(other.num_states),
      goal_states(other.goal_states),
      init_state(other.init_state) {
}

TransitionSystem::~TransitionSystem() {
}

unique_ptr<TransitionSystem> TransitionSystem::merge(
    const Labels &labels,
    const TransitionSystem &ts1,
    const TransitionSystem &ts2,
    utils::Verbosity verbosity) {
    if (verbosity >= utils::Verbosity::VERBOSE) {
        cout << "Merging " << ts1.get_description() << " and "
             << ts2.get_description() << endl;
    }

    assert(ts1.init_state != PRUNED_STATE && ts2.init_state != PRUNED_STATE);
    assert(ts1.are_transitions_sorted_unique() && ts2.are_transitions_sorted_unique());

    int num_variables = ts1.num_variables;
    vector<int> incorporated_variables;
    ::set_union(
        ts1.incorporated_variables.begin(), ts1.incorporated_variables.end(),
        ts2.incorporated_variables.begin(), ts2.incorporated_variables.end(),
        back_inserter(incorporated_variables));
    vector<vector<int>> label_groups;
    vector<vector<Transition>> transitions_by_group_id;
    transitions_by_group_id.reserve(labels.get_max_size());

    int ts1_size = ts1.get_size();
    int ts2_size = ts2.get_size();
    int num_states = ts1_size * ts2_size;
    vector<bool> goal_states(num_states, false);
    int init_state = -1;

    for (int s1 = 0; s1 < ts1_size; ++s1) {
        for (int s2 = 0; s2 < ts2_size; ++s2) {
            int state = s1 * ts2_size + s2;
            if (ts1.goal_states[s1] && ts2.goal_states[s2])
                goal_states[state] = true;
            if (s1 == ts1.init_state && s2 == ts2.init_state)
                init_state = state;
        }
    }
    assert(init_state != -1);

    /*
      We can compute the local equivalence relation of a composite T
      from the local equivalence relations of the two components T1 and T2:
      l and l' are locally equivalent in T iff:
      (A) they are locally equivalent in T1 and in T2, or
      (B) they are both dead in T (e.g., this includes the case where
          l is dead in T1 only and l' is dead in T2 only, so they are not
          locally equivalent in either of the components).
    */
    int multiplier = ts2_size;
    vector<int> dead_labels;
    for (GroupAndTransitions gat : ts1) {
        const LabelGroup &group1 = gat.label_group;
        const vector<Transition> &transitions1 = gat.transitions;

        // Distribute the labels of this group among the "buckets"
        // corresponding to the groups of ts2.
        unordered_map<int, vector<int>> buckets;
        for (int label_no : group1) {
            int group2_id = ts2.label_equivalence_relation->get_group_id(label_no);
            buckets[group2_id].push_back(label_no);
        }
        // Now buckets contains all equivalence classes that are
        // refinements of group1.

        // Now create the new groups together with their transitions.
        for (auto &bucket : buckets) {
            const vector<Transition> &transitions2 =
                ts2.get_transitions_for_group_id(bucket.first);

            // Create the new transitions for this bucket
            vector<Transition> new_transitions;
            if (!transitions1.empty() && !transitions2.empty()
                && transitions1.size() > new_transitions.max_size() / transitions2.size())
                utils::exit_with(ExitCode::SEARCH_OUT_OF_MEMORY);
            new_transitions.reserve(transitions1.size() * transitions2.size());
            for (const Transition &transition1 : transitions1) {
                int src1 = transition1.src;
                int target1 = transition1.target;
                for (const Transition &transition2 : transitions2) {
                    int src2 = transition2.src;
                    int target2 = transition2.target;
                    int src = src1 * multiplier + src2;
                    int target = target1 * multiplier + target2;
                    new_transitions.push_back(Transition(src, target));
                }
            }

            // Create a new group if the transitions are not empty
            vector<int> &new_labels = bucket.second;
            if (new_transitions.empty()) {
                dead_labels.insert(dead_labels.end(), new_labels.begin(), new_labels.end());
            } else {
                sort(new_transitions.begin(), new_transitions.end());
                label_groups.push_back(move(new_labels));
                transitions_by_group_id.push_back(move(new_transitions));
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
        label_groups.push_back(move(dead_labels));
        // Dead labels have empty transitions
        transitions_by_group_id.emplace_back();
    }

    assert(transitions_by_group_id.size() == label_groups.size());

    unique_ptr<LabelEquivalenceRelation> label_equivalence_relation =
        utils::make_unique_ptr<LabelEquivalenceRelation>(labels, label_groups);

    return utils::make_unique_ptr<TransitionSystem>(
        num_variables,
        move(incorporated_variables),
        move(label_equivalence_relation),
        move(transitions_by_group_id),
        num_states,
        move(goal_states),
        init_state
        );
}

void TransitionSystem::compute_locally_equivalent_labels() {
    /*
      Compare every group of labels and their transitions to all others and
      merge two groups whenever the transitions are the same.
    */
    for (int group_id1 = 0; group_id1 < label_equivalence_relation->get_size();
         ++group_id1) {
        if (!label_equivalence_relation->is_empty_group(group_id1)) {
            const vector<Transition> &transitions1 = transitions_by_group_id[group_id1];
            for (int group_id2 = group_id1 + 1;
                 group_id2 < label_equivalence_relation->get_size(); ++group_id2) {
                if (!label_equivalence_relation->is_empty_group(group_id2)) {
                    vector<Transition> &transitions2 = transitions_by_group_id[group_id2];
                    if (transitions1 == transitions2) {
                        label_equivalence_relation->move_group_into_group(
                            group_id2, group_id1);
                        utils::release_vector_memory(transitions2);
                    }
                }
            }
        }
    }
}

void TransitionSystem::apply_abstraction(
    const StateEquivalenceRelation &state_equivalence_relation,
    const vector<int> &abstraction_mapping,
    utils::Verbosity verbosity) {
    assert(are_transitions_sorted_unique());
    assert(in_sync_with_label_equivalence_relation());

    int new_num_states = state_equivalence_relation.size();
    assert(new_num_states < num_states);
    if (verbosity >= utils::Verbosity::VERBOSE) {
        cout << tag() << "applying abstraction (" << get_size()
             << " to " << new_num_states << " states)" << endl;
    }

    vector<bool> new_goal_states(new_num_states, false);
    for (int new_state = 0; new_state < new_num_states; ++new_state) {
        const StateEquivalenceClass &state_equivalence_class =
            state_equivalence_relation[new_state];
        assert(!state_equivalence_class.empty());

        for (int old_state : state_equivalence_class) {
            if (goal_states[old_state]) {
                new_goal_states[new_state] = true;
                break;
            }
        }
    }
    goal_states = move(new_goal_states);

    // Update all transitions.
    for (vector<Transition> &transitions : transitions_by_group_id) {
        if (!transitions.empty()) {
            vector<Transition> new_transitions;
            /*
              We reserve more memory than necessary here, but this is better
              than potentially resizing the vector several times when inserting
              transitions one after the other. See issue604-v6.

              An alternative could be to not use a new vector, but to modify
              the existing transitions inplace, and to remove all empty
              positions in the end. This would be more ugly, though.
            */
            new_transitions.reserve(transitions.size());
            for (size_t i = 0; i < transitions.size(); ++i) {
                const Transition &transition = transitions[i];
                int src = abstraction_mapping[transition.src];
                int target = abstraction_mapping[transition.target];
                if (src != PRUNED_STATE && target != PRUNED_STATE)
                    new_transitions.push_back(Transition(src, target));
            }
            normalize_given_transitions(new_transitions);
            transitions = move(new_transitions);
        }
    }

    compute_locally_equivalent_labels();

    num_states = new_num_states;
    init_state = abstraction_mapping[init_state];
    if (verbosity >= utils::Verbosity::VERBOSE && init_state == PRUNED_STATE) {
        cout << tag() << "initial state pruned; task unsolvable" << endl;
    }

    assert(are_transitions_sorted_unique());
    assert(in_sync_with_label_equivalence_relation());
}

void TransitionSystem::apply_label_reduction(
    const vector<pair<int, vector<int>>> &label_mapping,
    bool only_equivalent_labels) {
    assert(are_transitions_sorted_unique());
    assert(in_sync_with_label_equivalence_relation());

    /*
      We iterate over the given label mapping, treating every new label and
      the reduced old labels separately. We further distinguish the case
      where we know that the reduced labels are all from the same equivalence
      class from the case where we may combine arbitrary labels.

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

    if (only_equivalent_labels) {
        label_equivalence_relation->apply_label_mapping(label_mapping);
    } else {
        /*
          Go over all mappings, collect transitions of old groups and
          remember all affected group IDs. This needs to happen *before*
          updating label_equivalence_relation, because after updating it,
          we cannot find out the group ID of reduced labels anymore.
        */
        vector<vector<Transition>> new_transitions;
        new_transitions.reserve(label_mapping.size());
        unordered_set<int> affected_group_ids;
        for (const pair<int, vector<int>> &mapping: label_mapping) {
            const vector<int> &old_label_nos = mapping.second;
            assert(old_label_nos.size() >= 2);
            unordered_set<int> seen_group_ids;
            set<Transition> new_label_transitions;
            for (int old_label_no : old_label_nos) {
                int group_id = label_equivalence_relation->get_group_id(old_label_no);
                if (seen_group_ids.insert(group_id).second) {
                    affected_group_ids.insert(group_id);
                    const vector<Transition> &transitions = transitions_by_group_id[group_id];
                    new_label_transitions.insert(transitions.begin(), transitions.end());
                }
            }
            new_transitions.emplace_back(
                new_label_transitions.begin(), new_label_transitions.end());
        }
        assert(label_mapping.size() == new_transitions.size());

        /*
           Apply all label mappings to label_equivalence_relation. This needs
           to happen *before* we can add the new transitions to this transition
           systems and *before* we can remove empty groups of old labels,
           because only after updating label_equivalence_relation, we know the
           group ID of the new labels and which old groups became empty.
        */
        label_equivalence_relation->apply_label_mapping(label_mapping, &affected_group_ids);

        /*
          Go over the transitions of new labels and add them at the correct
          position.

          NOTE: it is important that this happens in increasing order of label
          numbers to ensure that transitions_by_group_id are synchronized with
          label groups of label_equivalence_relation.
        */
        for (size_t i = 0; i < label_mapping.size(); ++i) {
            vector<Transition> &transitions = new_transitions[i];
            assert(label_equivalence_relation->get_group_id(label_mapping[i].first)
                   == static_cast<int>(transitions_by_group_id.size()));
            transitions_by_group_id.push_back(move(transitions));
        }

        // Go over all affected group IDs and remove their transitions if the
        // group is empty.
        for (int group_id : affected_group_ids) {
            if (label_equivalence_relation->is_empty_group(group_id)) {
                utils::release_vector_memory(transitions_by_group_id[group_id]);
            }
        }

        compute_locally_equivalent_labels();
    }

    assert(are_transitions_sorted_unique());
    assert(in_sync_with_label_equivalence_relation());
}

string TransitionSystem::tag() const {
    string desc(get_description());
    desc[0] = toupper(desc[0]);
    return desc + ": ";
}

bool TransitionSystem::are_transitions_sorted_unique() const {
    for (GroupAndTransitions gat : *this) {
        if (!utils::is_sorted_unique(gat.transitions))
            return false;
    }
    return true;
}

bool TransitionSystem::in_sync_with_label_equivalence_relation() const {
    return label_equivalence_relation->get_size() ==
           static_cast<int>(transitions_by_group_id.size());
}

bool TransitionSystem::is_solvable(const Distances &distances) const {
    if (init_state == PRUNED_STATE) {
        return false;
    }
    if (distances.are_goal_distances_computed() && distances.get_goal_distance(init_state) == INF) {
        return false;
    }
    return true;
}

int TransitionSystem::compute_total_transitions() const {
    int total = 0;
    for (GroupAndTransitions gat : *this) {
        total += gat.transitions.size();
    }
    return total;
}

string TransitionSystem::get_description() const {
    ostringstream s;
    if (incorporated_variables.size() == 1) {
        s << "atomic transition system #" << *incorporated_variables.begin();
    } else {
        s << "composite transition system with "
          << incorporated_variables.size() << "/" << num_variables << " vars";
    }
    return s.str();
}

void TransitionSystem::dump_dot_graph() const {
    assert(are_transitions_sorted_unique());
    cout << "digraph transition_system";
    for (size_t i = 0; i < incorporated_variables.size(); ++i)
        cout << "_" << incorporated_variables[i];
    cout << " {" << endl;
    cout << "    node [shape = none] start;" << endl;
    for (int i = 0; i < num_states; ++i) {
        bool is_init = (i == init_state);
        bool is_goal = goal_states[i];
        cout << "    node [shape = " << (is_goal ? "doublecircle" : "circle")
             << "] node" << i << ";" << endl;
        if (is_init)
            cout << "    start -> node" << i << ";" << endl;
    }
    for (GroupAndTransitions gat : *this) {
        const LabelGroup &label_group = gat.label_group;
        const vector<Transition> &transitions = gat.transitions;
        for (const Transition &transition : transitions) {
            int src = transition.src;
            int target = transition.target;
            cout << "    node" << src << " -> node" << target << " [label = ";
            for (LabelConstIter label_it = label_group.begin();
                 label_it != label_group.end(); ++label_it) {
                if (label_it != label_group.begin())
                    cout << "_";
                cout << "x" << *label_it;
            }
            cout << "];" << endl;
        }
    }
    cout << "}" << endl;
}

void TransitionSystem::dump_labels_and_transitions() const {
    cout << tag() << "transitions" << endl;
    for (GroupAndTransitions gat : *this) {
        const LabelGroup &label_group = gat.label_group;
//        cout << "group ID: " << ts_it.get_id() << endl;
        cout << "labels: ";
        for (LabelConstIter label_it = label_group.begin();
             label_it != label_group.end(); ++label_it) {
            if (label_it != label_group.begin())
                cout << ",";
            cout << *label_it;
        }
        cout << endl;
        cout << "transitions: ";
        const vector<Transition> &transitions = gat.transitions;
        for (size_t i = 0; i < transitions.size(); ++i) {
            int src = transitions[i].src;
            int target = transitions[i].target;
            if (i != 0)
                cout << ",";
            cout << src << " -> " << target;
        }
        cout << endl;
        cout << "cost: " << label_group.get_cost() << endl;
    }
}

void TransitionSystem::statistics() const {
    cout << tag() << get_size() << " states, "
         << compute_total_transitions() << " arcs " << endl;
}
}
