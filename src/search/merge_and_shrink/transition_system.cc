#include "transition_system.h"

#include "distances.h"
#include "global_labels.h"

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
    const vector<vector<int>> &local_to_global_label_nos,
    const vector<vector<Transition>> &transitions_by_local_label_no,
    const vector<int> &local_label_no_to_cost,
    bool end)
    : local_to_global_label_nos(local_to_global_label_nos),
      transitions_by_local_label_no(transitions_by_local_label_no),
      local_label_no_to_cost(local_label_no_to_cost),
      current_label_no((end ? local_to_global_label_nos.size() : 0)) {
}

void TSConstIterator::operator++() {
    ++current_label_no;
}

GroupAndTransitions TSConstIterator::operator*() const {
    return GroupAndTransitions(
        local_to_global_label_nos[current_label_no],
        transitions_by_local_label_no[current_label_no],
        local_label_no_to_cost[current_label_no]);
}


/*
  Implementation note: Transitions are grouped by their (local) labels,
  not by source state or any such thing. Such a grouping is beneficial
  for fast generation of products because we can iterate label by label
  (and the global labels they represent), and it also allows applying
  transition system mappings very efficiently.

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
    const GlobalLabels &global_labels,
    vector<int> &&global_to_local_label_nos,
    vector<vector<int>> &&local_to_global_label_nos,
    vector<vector<Transition>> &&transitions_by_local_label_no,
    vector<int> &&local_label_no_to_cost,
    int num_states,
    vector<bool> &&goal_states,
    int init_state)
    : num_variables(num_variables),
      incorporated_variables(move(incorporated_variables)),
      global_labels(move(global_labels)),
      global_to_local_label_nos(move(global_to_local_label_nos)),
      local_to_global_label_nos(move(local_to_global_label_nos)),
      transitions_by_local_label_no(move(transitions_by_local_label_no)),
      local_label_no_to_cost(move(local_label_no_to_cost)),
      num_states(num_states),
      goal_states(move(goal_states)),
      init_state(init_state) {
    assert(this->local_to_global_label_nos.size() == this->transitions_by_local_label_no.size());
    assert(this->local_to_global_label_nos.size() == this->local_label_no_to_cost.size());
    assert(is_valid());
}

TransitionSystem::TransitionSystem(const TransitionSystem &other)
    : num_variables(other.num_variables),
      incorporated_variables(other.incorporated_variables),
      global_labels(other.global_labels),
      global_to_local_label_nos(other.global_to_local_label_nos),
      local_to_global_label_nos(other.local_to_global_label_nos),
      transitions_by_local_label_no(other.transitions_by_local_label_no),
      local_label_no_to_cost(other.local_label_no_to_cost),
      num_states(other.num_states),
      goal_states(other.goal_states),
      init_state(other.init_state) {
}

TransitionSystem::~TransitionSystem() {
}

unique_ptr<TransitionSystem> TransitionSystem::merge(
    const GlobalLabels &global_labels,
    const TransitionSystem &ts1,
    const TransitionSystem &ts2,
    utils::Verbosity verbosity) {
    if (verbosity >= utils::Verbosity::VERBOSE) {
        cout << "Merging " << ts1.get_description() << " and "
             << ts2.get_description() << endl;
    }

    assert(ts1.init_state != PRUNED_STATE && ts2.init_state != PRUNED_STATE);
    assert(ts1.is_valid() && ts2.is_valid());

    int num_variables = ts1.num_variables;
    vector<int> incorporated_variables;
    ::set_union(
        ts1.incorporated_variables.begin(), ts1.incorporated_variables.end(),
        ts2.incorporated_variables.begin(), ts2.incorporated_variables.end(),
        back_inserter(incorporated_variables));
    vector<int> global_to_local_label_nos(global_labels.get_max_size(), -1);
    vector<vector<int>> local_to_global_label_nos;
    vector<vector<Transition>> transitions_by_local_label_no;
    vector<int> local_label_no_to_cost;
    int max_num_local_labels_product =
        min(static_cast<int>(ts1.local_label_no_to_cost.size()) *
            static_cast<int>(ts2.local_label_no_to_cost.size()),
            global_labels.get_num_active_labels());
    local_to_global_label_nos.reserve(max_num_local_labels_product);
    transitions_by_local_label_no.reserve(max_num_local_labels_product);
    local_label_no_to_cost.reserve(max_num_local_labels_product);

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
            int ts_local_label_no2 = ts2.global_to_local_label_nos[label_no];
            buckets[ts_local_label_no2].push_back(label_no);
        }
        // Now buckets contains all equivalence classes that are
        // refinements of group1.

        // Now create the new groups together with their transitions.
        for (auto &bucket : buckets) {
            const vector<Transition> &transitions2 =
                ts2.get_transitions_for_local_label_no(bucket.first);

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
                int new_local_label_no = transitions_by_local_label_no.size();
                int cost = INF;
                for (int label_no : new_labels) {
                    cost = min(ts1.global_labels.get_label_cost(label_no), cost);
                    global_to_local_label_nos[label_no] = new_local_label_no;
                }
                local_to_global_label_nos.push_back(move(new_labels));
                transitions_by_local_label_no.push_back(move(new_transitions));
                local_label_no_to_cost.push_back(cost);
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
        int new_local_label_no = transitions_by_local_label_no.size();
        int cost = INF;
        for (int label_no : dead_labels) {
            cost = min(cost, ts1.global_labels.get_label_cost(label_no));
            global_to_local_label_nos[label_no] = new_local_label_no;
        }
        local_to_global_label_nos.push_back(move(dead_labels));
        // Dead labels have empty transitions
        transitions_by_local_label_no.emplace_back();
        local_label_no_to_cost.push_back(cost);
    }

    assert(static_cast<int>(local_label_no_to_cost.size()) <= max_num_local_labels_product);
    assert(transitions_by_local_label_no.size() == local_to_global_label_nos.size());

    return utils::make_unique_ptr<TransitionSystem>(
        num_variables,
        move(incorporated_variables),
        ts1.global_labels,
        move(global_to_local_label_nos),
        move(local_to_global_label_nos),
        move(transitions_by_local_label_no),
        move(local_label_no_to_cost),
        num_states,
        move(goal_states),
        init_state
        );
}

void TransitionSystem::make_local_labels_contiguous() {
    int num_local_labels = local_to_global_label_nos.size();
    int new_num_local_labels = 0;
    for (int local_label_no = 0; local_label_no < num_local_labels; ++local_label_no) {
        if (!local_to_global_label_nos[local_label_no].empty()) {
            ++new_num_local_labels;
        }
    }

    if (new_num_local_labels < num_local_labels) {
        vector<vector<int>> new_local_to_global_label_nos;
        vector<vector<Transition>> new_transitions_by_local_label_no;
        vector<int> new_local_label_no_to_cost;
        new_local_to_global_label_nos.reserve(new_num_local_labels);
        new_transitions_by_local_label_no.reserve(new_num_local_labels);
        new_local_label_no_to_cost.reserve(new_num_local_labels);
        int new_local_label_no = 0;
        for (int local_label_no = 0; local_label_no < num_local_labels; ++local_label_no) {
            if (!local_to_global_label_nos[local_label_no].empty()) {
                for (int global_label_no : local_to_global_label_nos[local_label_no]) {
                    assert(global_to_local_label_nos[global_label_no] == local_label_no);
                    global_to_local_label_nos[global_label_no] = new_local_label_no;
                }
                new_local_to_global_label_nos.push_back(move(local_to_global_label_nos[local_label_no]));
                new_transitions_by_local_label_no.push_back(move(transitions_by_local_label_no[local_label_no]));
                new_local_label_no_to_cost.push_back(local_label_no_to_cost[local_label_no]);
                ++new_local_label_no;
            }
        }
        new_local_to_global_label_nos.swap(local_to_global_label_nos);
        new_transitions_by_local_label_no.swap(transitions_by_local_label_no);
        new_local_label_no_to_cost.swap(local_label_no_to_cost);
    }
}

void TransitionSystem::compute_locally_equivalent_labels() {
    /*
      Compare every group of labels and their transitions to all others and
      merge two groups whenever the transitions are the same.
    */
    for (size_t local_label_no1 = 0; local_label_no1 < transitions_by_local_label_no.size();
         ++local_label_no1) {
        if (!local_to_global_label_nos[local_label_no1].empty()) {
            const vector<Transition> &transitions1 = transitions_by_local_label_no[local_label_no1];
            for (size_t local_label_no2 = local_label_no1 + 1;
                 local_label_no2 < transitions_by_local_label_no.size(); ++local_label_no2) {
                if (!local_to_global_label_nos[local_label_no2].empty()) {
                    vector<Transition> &transitions2 = transitions_by_local_label_no[local_label_no2];
                    if (transitions1 == transitions2) {
                        for (int global_label_no : local_to_global_label_nos[local_label_no2]) {
                            global_to_local_label_nos[global_label_no] = local_label_no1;
                        }
                        local_to_global_label_nos[local_label_no1].insert(
                            local_to_global_label_nos[local_label_no1].end(),
                            make_move_iterator(local_to_global_label_nos[local_label_no2].begin()),
                            make_move_iterator(local_to_global_label_nos[local_label_no2].end()));
                        local_to_global_label_nos[local_label_no2].clear();
                        utils::release_vector_memory(transitions2);
                    }
                }
            }
        }
    }

    make_local_labels_contiguous();
}

void TransitionSystem::apply_abstraction(
    const StateEquivalenceRelation &state_equivalence_relation,
    const vector<int> &abstraction_mapping,
    utils::Verbosity verbosity) {
    assert(is_valid());

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
    for (vector<Transition> &transitions : transitions_by_local_label_no) {
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

    assert(is_valid());
}

void TransitionSystem::apply_label_reduction(
    const vector<pair<int, vector<int>>> &label_mapping,
    bool only_equivalent_labels) {
    /*
      We iterate over the given label mapping, treating every new label and
      the reduced old labels separately. We further distinguish the case
      where we know that the reduced labels are all from the same equivalence
      class from the case where we may combine arbitrary labels.

      The case where only equivalent labels are combined is simple: remove all
      old labels from the local label they belong to and add the new one to
      the local label instead.

      The other case is more involved: again remove all old labels from their
      local labels, and the local labels themselves if do not represent any
      global labels anymore. Also collect the transitions of all reduced
      labels. Add a new local label for every new label and assign the
      collected transitions to it. Recompute the cost of all affected local
      labels and re-compute locally equivalent labels.
    */

    if (only_equivalent_labels) {
        // Update both label mappings.
        for (const pair<int, vector<int>> &mapping : label_mapping) {
            int new_label_no = mapping.first;
            const vector<int> &old_label_nos = mapping.second;
            int local_label_no = global_to_local_label_nos[old_label_nos.front()];
            local_to_global_label_nos[local_label_no].push_back(new_label_no);
            global_to_local_label_nos[new_label_no] = local_label_no;

            for (int old_label_no : old_label_nos) {
                assert(global_to_local_label_nos[old_label_no] == local_label_no);
                for (vector<int>::const_iterator it = local_to_global_label_nos[local_label_no].begin();
                     it != local_to_global_label_nos[local_label_no].end(); ) {
                    if (*it == old_label_no) {
                        it = local_to_global_label_nos[local_label_no].erase(it);
                    } else {
                        ++it;
                    }
                }
                // Reset (TODO: not necessary).
                global_to_local_label_nos[old_label_no] = -1;
            }
        }
    } else {
        /*
          Go over all mappings, collect transitions of old local labels and
          remember all affected such labels. This needs to happen *before*
          updating the label mappings (because otherwise, we could not find
          the correct local label for a given reduced global label).
        */
        vector<vector<Transition>> new_transitions;
        new_transitions.reserve(label_mapping.size());
        unordered_set<int> affected_local_label_nos;
        for (const pair<int, vector<int>> &mapping: label_mapping) {
            const vector<int> &old_label_nos = mapping.second;
            assert(old_label_nos.size() >= 2);
            unordered_set<int> seen_local_label_nos;
            set<Transition> new_label_transitions;
            for (int old_label_no : old_label_nos) {
                int local_label_no = global_to_local_label_nos[old_label_no];
                if (seen_local_label_nos.insert(local_label_no).second) {
                    affected_local_label_nos.insert(local_label_no);
                    const vector<Transition> &transitions = transitions_by_local_label_no[local_label_no];
                    new_label_transitions.insert(transitions.begin(), transitions.end());
                }
            }
            new_transitions.emplace_back(
                new_label_transitions.begin(), new_label_transitions.end());
        }
        assert(label_mapping.size() == new_transitions.size());

        /*
           Apply all label mappings to the label mappings. This needs
           to happen *before* we can add the new transitions to this transition
           system and *before* we can remove transitions of now obsolete
           local labels because only after updating the label mappings, we
           know the local label of the new labels and which old local labels
           do not represent any global labels anymore.
        */
        // Update both label mappings.
        for (const pair<int, vector<int>> &mapping : label_mapping) {
            int new_label_no = mapping.first;
            const vector<int> &old_label_nos = mapping.second;
            int new_local_label_no = local_to_global_label_nos.size();
            local_to_global_label_nos.push_back({new_label_no});
            local_label_no_to_cost.push_back(global_labels.get_label_cost(new_label_no));
            global_to_local_label_nos[new_label_no] = new_local_label_no;

            for (int old_label_no : old_label_nos) {
                int old_local_label_no = global_to_local_label_nos[old_label_no];
                for (vector<int>::const_iterator it = local_to_global_label_nos[old_local_label_no].begin();
                     it != local_to_global_label_nos[old_local_label_no].end(); ) {
                    if (*it == old_label_no) {
                        it = local_to_global_label_nos[old_local_label_no].erase(it);
                    } else {
                        ++it;
                    }
                }
                // Reset (TODO: not necessary).
                global_to_local_label_nos[old_label_no] = -1;
            }
        }

        // Recompute the cost of all affected local labels.
        for (int local_label_no : affected_local_label_nos) {
            const LabelGroup &label_group = local_to_global_label_nos[local_label_no];
            local_label_no_to_cost[local_label_no] = INF;
            for (int label_no : label_group) {
                int cost = global_labels.get_label_cost(label_no);
                local_label_no_to_cost[local_label_no] = min(local_label_no_to_cost[local_label_no], cost);
            }
        }

        /*
          Go over the transitions of new labels and add them at the correct
          position.

          NOTE: it is important that this happens in increasing order of label
          numbers to ensure that transitions_by_local_label_no are synchronized
          with local_to_global_label_nos and local_label_no_to_cost.
        */
        for (size_t i = 0; i < label_mapping.size(); ++i) {
            vector<Transition> &transitions = new_transitions[i];
            assert(global_to_local_label_nos[label_mapping[i].first]
                == static_cast<int>(transitions_by_local_label_no.size()));
            transitions_by_local_label_no.push_back(move(transitions));
        }

        // Finally remove transitions of all local labels that do not represent
        // any global label anymore.
        for (int local_label_no : affected_local_label_nos) {
            if (local_to_global_label_nos[local_label_no].empty()) {
                utils::release_vector_memory(transitions_by_local_label_no[local_label_no]);
            }
        }

        compute_locally_equivalent_labels();
    }

    assert(is_valid());
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

bool TransitionSystem::is_valid() const {
    return are_transitions_sorted_unique()
        && local_to_global_label_nos.size() == local_label_no_to_cost.size()
        && local_to_global_label_nos.size() == transitions_by_local_label_no.size()
        && is_label_mapping_consistent();
}

bool TransitionSystem::is_label_mapping_consistent() const {
//    for (int label_no = 0; label_no < global_labels.get_size(); ++label_no) {
//        if (global_labels.is_current_label(label_no)) {
//            cout << "label " << label_no << " -> "
//                 << global_to_local_label_nos[label_no] << endl;
//        }
//    }
//    for (size_t local_label_no = 0; local_label_no < local_to_global_label_nos.size(); ++local_label_no) {
//        cout << "local label " << local_label_no << " <- " << local_to_global_label_nos[local_label_no] << endl;
//    }

    for (int label_no = 0; label_no < global_labels.get_size(); ++label_no) {
        if (global_labels.is_current_label(label_no)) {
            int local_label_no = global_to_local_label_nos[label_no];
            const vector<int> &global_labels = local_to_global_label_nos[local_label_no];
            assert(!local_to_global_label_nos[local_label_no].empty());

            if (find(global_labels.begin(),
                     global_labels.end(),
                     label_no)
                    == global_labels.end()) {
                return false;
            }
        }
    }

    for (size_t local_label_no = 0; local_label_no < local_to_global_label_nos.size(); ++local_label_no) {
        for (int global_label_no : local_to_global_label_nos[local_label_no]) {
            if (global_to_local_label_nos[global_label_no] != static_cast<int>(local_label_no)) {
                return false;
            }
        }
    }
    return true;
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
    assert(is_valid());
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
            for (size_t i = 0; i < label_group.size(); ++i) {
                if (i != 0)
                    cout << "_";
                cout << "x" << label_group[i];
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
        cout << "labels: " << label_group << endl;
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
        cout << "cost: " << gat.cost << endl;
    }
}

void TransitionSystem::statistics() const {
    cout << tag() << get_size() << " states, "
         << compute_total_transitions() << " arcs " << endl;
}
}
