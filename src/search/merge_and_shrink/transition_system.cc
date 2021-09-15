#include "transition_system.h"

#include "distances.h"
#include "global_labels.h"

#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <iostream>
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
    const vector<LabelGroup> &local_to_global_labels,
    const vector<vector<Transition>> &local_label_to_transitions,
    const vector<int> &local_label_to_cost,
    bool end)
    : local_to_global_labels(local_to_global_labels),
      local_label_to_transitions(local_label_to_transitions),
      local_label_to_cost(local_label_to_cost),
      current_label((end ? local_to_global_labels.size() : 0)) {
}

void TSConstIterator::operator++() {
    ++current_label;
}

TransitionGroup TSConstIterator::operator*() const {
    return TransitionGroup(
        local_to_global_labels[current_label],
        local_label_to_transitions[current_label],
        local_label_to_cost[current_label]);
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
    vector<int> &&global_to_local_label,
    vector<LabelGroup> &&local_to_global_labels,
    vector<vector<Transition>> &&local_label_to_transitions,
    vector<int> &&local_label_to_cost,
    int num_states,
    vector<bool> &&goal_states,
    int init_state)
    : num_variables(num_variables),
      incorporated_variables(move(incorporated_variables)),
      global_labels(move(global_labels)),
      global_to_local_label(move(global_to_local_label)),
      local_to_global_labels(move(local_to_global_labels)),
      local_label_to_transitions(move(local_label_to_transitions)),
      local_label_to_cost(move(local_label_to_cost)),
      num_states(num_states),
      goal_states(move(goal_states)),
      init_state(init_state) {
    assert(this->local_to_global_labels.size() == this->local_label_to_transitions.size());
    assert(this->local_to_global_labels.size() == this->local_label_to_cost.size());
    assert(is_valid());
}

TransitionSystem::TransitionSystem(const TransitionSystem &other)
    : num_variables(other.num_variables),
      incorporated_variables(other.incorporated_variables),
      global_labels(other.global_labels),
      global_to_local_label(other.global_to_local_label),
      local_to_global_labels(other.local_to_global_labels),
      local_label_to_transitions(other.local_label_to_transitions),
      local_label_to_cost(other.local_label_to_cost),
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
        utils::g_log << "Merging " << ts1.get_description() << " and "
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
    vector<int> global_to_local_label(global_labels.get_max_num_labels(), -1);
    vector<LabelGroup> local_to_global_labels;
    vector<vector<Transition>> local_label_to_transitions;
    vector<int> local_label_to_cost;
    int max_num_local_labels_product =
        min(static_cast<int>(ts1.local_label_to_cost.size()) *
            static_cast<int>(ts2.local_label_to_cost.size()),
            global_labels.get_num_active_labels());
    local_to_global_labels.reserve(max_num_local_labels_product);
    local_label_to_transitions.reserve(max_num_local_labels_product);
    local_label_to_cost.reserve(max_num_local_labels_product);

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
    LabelGroup dead_labels;
    for (TransitionGroup ts_group : ts1) {
        const LabelGroup &group1 = ts_group.label_group;
        const vector<Transition> &transitions1 = ts_group.transitions;

        // Distribute the labels of this group among the "buckets"
        // corresponding to the groups of ts2.
        unordered_map<int, LabelGroup> buckets;
        for (int label : group1) {
            int ts_local_label2 = ts2.global_to_local_label[label];
            buckets[ts_local_label2].push_back(label);
        }
        // Now buckets contains all equivalence classes that are
        // refinements of group1.

        // Now create the new groups together with their transitions.
        for (auto &bucket : buckets) {
            const vector<Transition> &transitions2 =
                ts2.local_label_to_transitions[bucket.first];

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
            LabelGroup &new_labels = bucket.second;
            if (new_transitions.empty()) {
                dead_labels.insert(dead_labels.end(), new_labels.begin(), new_labels.end());
            } else {
                sort(new_transitions.begin(), new_transitions.end());
                int new_local_label = local_label_to_transitions.size();
                int cost = INF;
                for (int label : new_labels) {
                    cost = min(ts1.global_labels.get_label_cost(label), cost);
                    global_to_local_label[label] = new_local_label;
                }
                local_to_global_labels.push_back(move(new_labels));
                local_label_to_transitions.push_back(move(new_transitions));
                local_label_to_cost.push_back(cost);
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
        int new_local_label = local_label_to_transitions.size();
        int cost = INF;
        for (int label : dead_labels) {
            cost = min(cost, ts1.global_labels.get_label_cost(label));
            global_to_local_label[label] = new_local_label;
        }
        local_to_global_labels.push_back(move(dead_labels));
        // Dead labels have empty transitions
        local_label_to_transitions.emplace_back();
        local_label_to_cost.push_back(cost);
    }

    assert(static_cast<int>(local_label_to_cost.size()) <= max_num_local_labels_product);
    assert(local_label_to_transitions.size() == local_to_global_labels.size());

    return utils::make_unique_ptr<TransitionSystem>(
        num_variables,
        move(incorporated_variables),
        ts1.global_labels,
        move(global_to_local_label),
        move(local_to_global_labels),
        move(local_label_to_transitions),
        move(local_label_to_cost),
        num_states,
        move(goal_states),
        init_state
        );
}

void TransitionSystem::make_local_labels_contiguous() {
    int num_local_labels = local_to_global_labels.size();
    int new_num_local_labels = 0;
    for (int local_label = 0; local_label < num_local_labels; ++local_label) {
        if (!local_to_global_labels[local_label].empty()) {
            ++new_num_local_labels;
        }
    }

    if (new_num_local_labels < num_local_labels) {
        vector<LabelGroup> new_local_to_global_labels;
        vector<vector<Transition>> new_local_label_to_transitions;
        vector<int> new_local_label_to_cost;
        new_local_to_global_labels.reserve(new_num_local_labels);
        new_local_label_to_transitions.reserve(new_num_local_labels);
        new_local_label_to_cost.reserve(new_num_local_labels);
        int new_local_label = 0;
        for (int local_label = 0; local_label < num_local_labels; ++local_label) {
            if (!local_to_global_labels[local_label].empty()) {
                for (int global_label : local_to_global_labels[local_label]) {
                    assert(global_to_local_label[global_label] == local_label);
                    global_to_local_label[global_label] = new_local_label;
                }
                new_local_to_global_labels.push_back(move(local_to_global_labels[local_label]));
                new_local_label_to_transitions.push_back(move(local_label_to_transitions[local_label]));
                new_local_label_to_cost.push_back(local_label_to_cost[local_label]);
                ++new_local_label;
            }
        }
        new_local_to_global_labels.swap(local_to_global_labels);
        new_local_label_to_transitions.swap(local_label_to_transitions);
        new_local_label_to_cost.swap(local_label_to_cost);
    }
}

void TransitionSystem::compute_locally_equivalent_labels() {
    /*
      Compare every group of labels and their transitions to all others and
      merge two groups whenever the transitions are the same.

      Note that there can be empty local label groups after applying label
      reduction when combining labels which are combinable for this transition
      system.
    */
    for (size_t local_label1 = 0; local_label1 < local_label_to_transitions.size();
         ++local_label1) {
        if (!local_to_global_labels[local_label1].empty()) {
            const vector<Transition> &transitions1 = local_label_to_transitions[local_label1];
            for (size_t local_label2 = local_label1 + 1;
                 local_label2 < local_label_to_transitions.size(); ++local_label2) {
                if (!local_to_global_labels[local_label2].empty()) {
                    vector<Transition> &transitions2 = local_label_to_transitions[local_label2];
                    // Comparing transitions directly works because they are sorted and unique.
                    if (transitions1 == transitions2) {
                        for (int global_label : local_to_global_labels[local_label2]) {
                            global_to_local_label[global_label] = local_label1;
                        }
                        local_to_global_labels[local_label1].insert(
                            local_to_global_labels[local_label1].end(),
                            make_move_iterator(local_to_global_labels[local_label2].begin()),
                            make_move_iterator(local_to_global_labels[local_label2].end()));
                        local_to_global_labels[local_label2].clear();
                        utils::release_vector_memory(transitions2);
                        local_label_to_cost[local_label1] = min(local_label_to_cost[local_label1], local_label_to_cost[local_label2]);
                        local_label_to_cost[local_label2] = -1; // for consistency
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
        utils::g_log << tag() << "applying abstraction (" << get_size()
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
    for (vector<Transition> &transitions : local_label_to_transitions) {
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
        utils::g_log << tag() << "initial state pruned; task unsolvable" << endl;
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
      old global labels from the local label they belong to (they all belong
      to the same local label because we know they are equivalent) and add the
      new global label to the local label instead. Thus the affected local
      label is "non-empty" in the sense that there are still global labels
      associated to it.

      The other case is more involved: again remove all old labels from their
      local labels, and the local labels themselves if they do not represent
      any global labels anymore. Also collect the transitions of all reduced
      labels. Add a new local label for every new global label and assign the
      collected transitions to it. Recompute the cost of all affected local
      labels and re-compute locally equivalent labels.
    */

    if (only_equivalent_labels) {
        // Update both label mappings.
        for (const pair<int, vector<int>> &mapping : label_mapping) {
            int new_label = mapping.first;
            const vector<int> &old_labels = mapping.second;
            assert(old_labels.size() >= 2);
            int local_label = global_to_local_label[old_labels.front()];
            LabelGroup &label_group = local_to_global_labels[local_label];
            label_group.push_back(new_label);
            global_to_local_label[new_label] = local_label;

            for (int old_label : old_labels) {
                assert(global_to_local_label[old_label] == local_label);
                label_group.erase(find(label_group.begin(), label_group.end(), old_label));
                // Reset (for consistency only, old labels are never accessed).
                global_to_local_label[old_label] = -1;
                // NOTE: if we were combining labels with different cost,
                // we would need to recompute the cost of the local label here.
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
        unordered_set<int> affected_local_labels;
        for (const pair<int, vector<int>> &mapping: label_mapping) {
            const vector<int> &old_labels = mapping.second;
            assert(old_labels.size() >= 2);
            unordered_set<int> seen_local_labels;
            // TODO: consider using unordered_set and sort once at the end.
            set<Transition> new_label_transitions;
            for (int old_label : old_labels) {
                int local_label = global_to_local_label[old_label];
                if (seen_local_labels.insert(local_label).second) {
                    affected_local_labels.insert(local_label);
                    const vector<Transition> &transitions = local_label_to_transitions[local_label];
                    new_label_transitions.insert(transitions.begin(), transitions.end());
                }
            }
            new_transitions.emplace_back(
                new_label_transitions.begin(), new_label_transitions.end());
        }
        assert(label_mapping.size() == new_transitions.size());

        /*
           Apply all label reductions to the label mappings (data structures).
           This needs to happen *before* we can add the new transitions to this
           transition system and *before* we can remove transitions of now
           obsolete local labels because only after updating the label
           mappings, we know the local label of the new global labels and which
           old local labels do not represent any global labels anymore.
        */
        // Update both label mappings.
        for (const pair<int, vector<int>> &mapping : label_mapping) {
            int new_label = mapping.first;
            const vector<int> &old_labels = mapping.second;
            int new_local_label = local_to_global_labels.size();
            local_to_global_labels.push_back({new_label});
            local_label_to_cost.push_back(global_labels.get_label_cost(new_label));
            global_to_local_label[new_label] = new_local_label;

            for (int old_label : old_labels) {
                int old_local_label = global_to_local_label[old_label];
                LabelGroup &label_group = local_to_global_labels[old_local_label];
                label_group.erase(find(label_group.begin(), label_group.end(), old_label));
                // Reset (for consistency only, old labels are never accessed).
                global_to_local_label[old_label] = -1;
            }
        }

        // Recompute the cost of all affected local labels.
        for (int local_label : affected_local_labels) {
            const LabelGroup &label_group = local_to_global_labels[local_label];
            local_label_to_cost[local_label] = INF;
            for (int label : label_group) {
                int cost = global_labels.get_label_cost(label);
                local_label_to_cost[local_label] = min(local_label_to_cost[local_label], cost);
            }
        }

        /*
          Go over the transitions of new global labels and add them at the
          correct position.

          NOTE: it is important that this happens in increasing order of label
          numbers to ensure that local_label_to_transitions are synchronized
          with local_to_global_labels and local_label_to_cost.
        */
        for (size_t i = 0; i < label_mapping.size(); ++i) {
            vector<Transition> &transitions = new_transitions[i];
            // Assert that the new local label id is correctly set.
            assert(global_to_local_label[label_mapping[i].first]
                   == static_cast<int>(local_label_to_transitions.size()));
            local_label_to_transitions.push_back(move(transitions));
        }

        // Finally remove transitions of all local labels that do not represent
        // any global label anymore.
        for (int local_label : affected_local_labels) {
            if (local_to_global_labels[local_label].empty()) {
                utils::release_vector_memory(local_label_to_transitions[local_label]);
                local_label_to_cost[local_label] = -1; // for consistency
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
    for (TransitionGroup ts_group : *this) {
        if (!utils::is_sorted_unique(ts_group.transitions))
            return false;
    }
    return true;
}

bool TransitionSystem::is_valid() const {
    return are_transitions_sorted_unique()
           && local_to_global_labels.size() == local_label_to_cost.size()
           && local_to_global_labels.size() == local_label_to_transitions.size()
           && is_label_mapping_consistent();
}

bool TransitionSystem::is_label_mapping_consistent() const {
    for (int global_label : global_labels) {
        int local_label = global_to_local_label[global_label];
        const LabelGroup &global_labels = local_to_global_labels[local_label];
        assert(!global_labels.empty());

        if (find(global_labels.begin(),
                 global_labels.end(),
                 global_label)
            == global_labels.end()) {
            dump_label_mapping();
            cerr << "global label " << global_label << " is not part of the "
                "local label it is mapped to" << endl;
            return false;
        }
    }

    for (size_t local_label = 0; local_label < local_to_global_labels.size(); ++local_label) {
        for (int global_label : local_to_global_labels[local_label]) {
            if (global_to_local_label[global_label] != static_cast<int>(local_label)) {
                dump_label_mapping();
                cerr << "global label " << global_label << " is not mapped "
                    "to the local label it is part of" << endl;
                return false;
            }
        }
    }
    return true;
}

void TransitionSystem::dump_label_mapping() const {
    utils::g_log << "global to local label mapping: ";
    for (int global_label : global_labels) {
        utils::g_log << global_label << " -> "
                     << global_to_local_label[global_label] << ", ";
    }
    utils::g_log << endl;
    utils::g_log << "local to global label mapping: ";
    for (size_t local_label = 0;
         local_label < local_to_global_labels.size(); ++local_label) {
        utils::g_log << local_label << ": "
                     << local_to_global_labels[local_label] << ", ";
    }
    utils::g_log << endl;
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
    for (TransitionGroup ts_group : *this) {
        total += ts_group.transitions.size();
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
    utils::g_log << "digraph transition_system";
    for (size_t i = 0; i < incorporated_variables.size(); ++i)
        utils::g_log << "_" << incorporated_variables[i];
    utils::g_log << " {" << endl;
    utils::g_log << "    node [shape = none] start;" << endl;
    for (int i = 0; i < num_states; ++i) {
        bool is_init = (i == init_state);
        bool is_goal = goal_states[i];
        utils::g_log << "    node [shape = " << (is_goal ? "doublecircle" : "circle")
                     << "] node" << i << ";" << endl;
        if (is_init)
            utils::g_log << "    start -> node" << i << ";" << endl;
    }
    for (TransitionGroup ts_group : *this) {
        const LabelGroup &label_group = ts_group.label_group;
        const vector<Transition> &transitions = ts_group.transitions;
        for (const Transition &transition : transitions) {
            int src = transition.src;
            int target = transition.target;
            utils::g_log << "    node" << src << " -> node" << target << " [label = ";
            for (size_t i = 0; i < label_group.size(); ++i) {
                if (i != 0)
                    utils::g_log << "_";
                utils::g_log << "x" << label_group[i];
            }
            utils::g_log << "];" << endl;
        }
    }
    utils::g_log << "}" << endl;
}

void TransitionSystem::dump_labels_and_transitions() const {
    utils::g_log << tag() << "transitions" << endl;
    for (TransitionGroup ts_group : *this) {
        const LabelGroup &label_group = ts_group.label_group;
        utils::g_log << "labels: " << label_group << endl;
        utils::g_log << "transitions: ";
        const vector<Transition> &transitions = ts_group.transitions;
        for (size_t i = 0; i < transitions.size(); ++i) {
            int src = transitions[i].src;
            int target = transitions[i].target;
            if (i != 0)
                utils::g_log << ",";
            utils::g_log << src << " -> " << target;
        }
        utils::g_log << endl;
        utils::g_log << "cost: " << ts_group.cost << endl;
    }
}

void TransitionSystem::statistics() const {
    utils::g_log << tag() << get_size() << " states, "
                 << compute_total_transitions() << " arcs " << endl;
}
}
