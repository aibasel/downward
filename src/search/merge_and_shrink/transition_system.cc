#include "transition_system.h"

#include "distances.h"
#include "labels.h"

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

void LocalLabelInfo::add_label(int label, int label_cost) {
    label_group.push_back(label);
    if (label_cost != -1) {
        cost = min(cost, label_cost);
    }
    assert(is_consistent());
}

void LocalLabelInfo::apply_same_cost_label_mapping(
    int new_label, const vector<int> &old_labels) {
    assert(is_consistent());
    remove_labels(old_labels);
    label_group.push_back(new_label);
    assert(is_consistent());
}

void LocalLabelInfo::remove_labels(const vector<int> &old_labels) {
    assert(is_consistent());
    assert(utils::is_sorted_unique(old_labels));
    auto it = set_difference(
        label_group.begin(), label_group.end(),
        old_labels.begin(), old_labels.end(),
        label_group.begin());
    label_group.erase(it, label_group.end());
    assert(is_consistent());
}

void LocalLabelInfo::recompute_cost(const Labels &labels) {
    cost = INF;
    for (int label : label_group) {
        int label_cost = labels.get_label_cost(label);
        cost = min(cost, label_cost);
    }
}

void LocalLabelInfo::replace_transitions(vector<Transition> &&new_transitions) {
    transitions = move(new_transitions);
    assert(is_consistent());
}

void LocalLabelInfo::merge_local_label_info(LocalLabelInfo &local_label_info) {
    assert(is_consistent());
    assert(local_label_info.is_consistent());
    assert(transitions == local_label_info.transitions);
    label_group.insert(
        label_group.end(),
        make_move_iterator(local_label_info.label_group.begin()),
        make_move_iterator(local_label_info.label_group.end()));
    cost = min(cost, local_label_info.cost);
    local_label_info.deactivate();
    sort(label_group.begin(), label_group.end());
    assert(is_consistent());
}

void LocalLabelInfo::deactivate() {
    utils::release_vector_memory(transitions);
    utils::release_vector_memory(label_group);
    cost = -1;
}

bool LocalLabelInfo::is_consistent() const {
    return utils::is_sorted_unique(label_group) &&
           utils::is_sorted_unique(transitions);
}


TransitionSystemConstIterator::TransitionSystemConstIterator(
    vector<LocalLabelInfo>::const_iterator it,
    vector<LocalLabelInfo>::const_iterator end_it)
    : it(it), end_it(end_it) {
    advance_to_next_valid_index();
}

void TransitionSystemConstIterator::advance_to_next_valid_index() {
    while (it != end_it && !it->is_active()) {
        ++it;
    }
}

TransitionSystemConstIterator &TransitionSystemConstIterator::operator++() {
    ++it;
    advance_to_next_valid_index();
    return *this;
}

/*
  Implementation note: Transitions are grouped by their local labels,
  not by source state or any such thing. Such a grouping is beneficial
  for fast generation of products because we can iterate local label by
  local label (and the labels they represent), and it also allows applying
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
    const Labels &labels,
    vector<int> &&label_to_local_label,
    vector<LocalLabelInfo> &&local_label_infos,
    int num_states,
    vector<bool> &&goal_states,
    int init_state)
    : num_variables(num_variables),
      incorporated_variables(move(incorporated_variables)),
      labels(move(labels)),
      label_to_local_label(move(label_to_local_label)),
      local_label_infos(move(local_label_infos)),
      num_states(num_states),
      goal_states(move(goal_states)),
      init_state(init_state) {
    assert(is_valid());
}

TransitionSystem::TransitionSystem(const TransitionSystem &other)
    : num_variables(other.num_variables),
      incorporated_variables(other.incorporated_variables),
      labels(other.labels),
      label_to_local_label(other.label_to_local_label),
      local_label_infos(other.local_label_infos),
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
    utils::LogProxy &log) {
    if (log.is_at_least_verbose()) {
        log << "Merging " << ts1.get_description() << " and "
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
    vector<int> label_to_local_label(labels.get_max_num_labels(), -1);
    vector<LocalLabelInfo> local_label_infos;

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
    for (const LocalLabelInfo &local_label_info : ts1) {
        const LabelGroup &group1 = local_label_info.get_label_group();
        const vector<Transition> &transitions1 = local_label_info.get_transitions();

        // Distribute the labels of this group among the "buckets"
        // corresponding to the groups of ts2.
        unordered_map<int, LabelGroup> buckets;
        for (int label : group1) {
            int ts_local_label2 = ts2.label_to_local_label[label];
            buckets[ts_local_label2].push_back(label);
        }
        // Now buckets contains all equivalence classes that are
        // refinements of group1.

        // Now create the new groups together with their transitions.
        for (auto &bucket : buckets) {
            const vector<Transition> &transitions2 =
                ts2.local_label_infos[bucket.first].get_transitions();

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
                    new_transitions.emplace_back(src, target);
                }
            }

            // Create a new group if the transitions are not empty
            LabelGroup &new_labels = bucket.second;
            if (new_transitions.empty()) {
                dead_labels.insert(dead_labels.end(), new_labels.begin(), new_labels.end());
            } else {
                sort(new_transitions.begin(), new_transitions.end());
                sort(new_labels.begin(), new_labels.end());
                int new_local_label = local_label_infos.size();
                int cost = INF;
                for (int label : new_labels) {
                    cost = min(ts1.labels.get_label_cost(label), cost);
                    label_to_local_label[label] = new_local_label;
                }
                local_label_infos.emplace_back(move(new_labels), move(new_transitions), cost);
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
        sort(dead_labels.begin(), dead_labels.end());
        int new_local_label = local_label_infos.size();
        int cost = INF;
        for (int label : dead_labels) {
            cost = min(cost, ts1.labels.get_label_cost(label));
            label_to_local_label[label] = new_local_label;
        }
        // Dead labels have empty transitions
        local_label_infos.emplace_back(move(dead_labels), vector<Transition>(), cost);
    }

    return utils::make_unique_ptr<TransitionSystem>(
        num_variables,
        move(incorporated_variables),
        ts1.labels,
        move(label_to_local_label),
        move(local_label_infos),
        num_states,
        move(goal_states),
        init_state
        );
}

void TransitionSystem::compute_equivalent_local_labels() {
    /*
      Compare every group of labels and their transitions to all others and
      merge two groups whenever the transitions are the same.

      Note that there can be empty local label groups after applying label
      reduction when combining labels which are combinable for this transition
      system.
    */
    int num_local_labels = local_label_infos.size();
    for (int local_label1 = 0; local_label1 < num_local_labels;
         ++local_label1) {
        if (local_label_infos[local_label1].is_active()) {
            const vector<Transition> &transitions1 = local_label_infos[local_label1].get_transitions();
            for (int local_label2 = local_label1 + 1;
                 local_label2 < num_local_labels; ++local_label2) {
                if (local_label_infos[local_label2].is_active()) {
                    const vector<Transition> &transitions2 = local_label_infos[local_label2].get_transitions();
                    // Comparing transitions directly works because they are sorted and unique.
                    if (transitions1 == transitions2) {
                        for (int label : local_label_infos[local_label2].get_label_group()) {
                            label_to_local_label[label] = local_label1;
                        }
                        local_label_infos[local_label1].merge_local_label_info(
                            local_label_infos[local_label2]);
                    }
                }
            }
        }
    }

    assert(is_valid());
}

void TransitionSystem::apply_abstraction(
    const StateEquivalenceRelation &state_equivalence_relation,
    const vector<int> &abstraction_mapping,
    utils::LogProxy &log) {
    assert(is_valid());

    int new_num_states = state_equivalence_relation.size();
    assert(new_num_states < num_states);
    if (log.is_at_least_verbose()) {
        log << tag() << "applying abstraction (" << get_size()
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
    for (LocalLabelInfo &local_label_info : local_label_infos) {
        const vector<Transition> &transitions = local_label_info.get_transitions();
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
            for (const Transition &transition : transitions) {
                int src = abstraction_mapping[transition.src];
                int target = abstraction_mapping[transition.target];
                if (src != PRUNED_STATE && target != PRUNED_STATE)
                    new_transitions.emplace_back(src, target);
            }
            utils::sort_unique(new_transitions);
            local_label_info.replace_transitions(move(new_transitions));
        }
    }

    compute_equivalent_local_labels();

    num_states = new_num_states;
    init_state = abstraction_mapping[init_state];
    if (log.is_at_least_verbose() && init_state == PRUNED_STATE) {
        log << tag() << "initial state pruned; task unsolvable" << endl;
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
      old labels from the local label they belong to (they all belong
      to the same local label because we know they are equivalent) and add the
      new label to the local label instead. Thus the affected local
      label is "non-empty" in the sense that there are still labels
      associated to it.

      The other case is more involved: again remove all old labels from their
      local labels, and the local labels themselves if they do not represent
      any labels anymore. Also collect the transitions of all reduced
      labels. Add a new local label for every new label and assign the
      collected transitions to it. Recompute the cost of all affected local
      labels and re-compute locally equivalent labels.
    */

    if (only_equivalent_labels) {
        // Update both label mappings.
        for (const pair<int, vector<int>> &mapping : label_mapping) {
            int new_label = mapping.first;
            const vector<int> &old_labels = mapping.second;
            assert(old_labels.size() >= 2);
            int local_label = label_to_local_label[old_labels.front()];
            local_label_infos[local_label].apply_same_cost_label_mapping(
                new_label, old_labels);

            label_to_local_label[new_label] = local_label;
            for (int old_label : old_labels) {
                assert(label_to_local_label[old_label] == local_label);
                // Reset (for consistency only, old labels are never accessed).
                label_to_local_label[old_label] = -1;
            }
        }
    } else {
        /*
          Iterate over the label mapping. For each new label, go over the
          reduced labels to combine their transitions into the transitions
          of the new label. Also store, for each local label, the labels
          removed from them. Add the new label together with its transitions
          as a new local label and update the label_to_local_label mapping.
        */
        unordered_map<int, vector<int>> local_label_to_old_labels;
        for (const pair<int, vector<int>> &mapping: label_mapping) {
            const vector<int> &old_labels = mapping.second;
            assert(old_labels.size() >= 2);
            unordered_set<int> seen_local_labels;
            vector<Transition> new_label_transitions;
            for (int old_label : old_labels) {
                int old_local_label = label_to_local_label[old_label];
                if (seen_local_labels.insert(old_local_label).second) {
                    const vector<Transition> &transitions = local_label_infos[old_local_label].get_transitions();
                    new_label_transitions.insert(new_label_transitions.end(), transitions.begin(), transitions.end());
                }
                local_label_to_old_labels[old_local_label].push_back(old_label);
                // Reset (for consistency only, old labels are never accessed).
                label_to_local_label[old_label] = -1;
            }
            utils::sort_unique(new_label_transitions);

            int new_label = mapping.first;
            int new_local_label = local_label_infos.size();
            label_to_local_label[new_label] = new_local_label;
            int new_cost = labels.get_label_cost(new_label);

            LabelGroup new_label_group = {new_label};
            local_label_infos.emplace_back(move(new_label_group), move(new_label_transitions), new_cost);
        }

        /*
          Remove all labels of all affected local labels and recompute the
          cost of these affected local labels.
        */
        for (auto &entry : local_label_to_old_labels) {
            sort(entry.second.begin(), entry.second.end());
            local_label_infos[entry.first].remove_labels(entry.second);
            local_label_infos[entry.first].recompute_cost(labels);
        }

        compute_equivalent_local_labels();
    }

    assert(is_valid());
}

string TransitionSystem::tag() const {
    string desc(get_description());
    desc[0] = toupper(desc[0]);
    return desc + ": ";
}

bool TransitionSystem::are_local_labels_consistent() const {
    for (const LocalLabelInfo &local_label_info : *this) {
        if (!local_label_info.is_consistent())
            return false;
    }
    return true;
}

bool TransitionSystem::is_valid() const {
    return are_local_labels_consistent()
           && is_label_mapping_consistent();
}

bool TransitionSystem::is_label_mapping_consistent() const {
    for (int label : labels) {
        int local_label = label_to_local_label[label];
        const LabelGroup &label_group = local_label_infos[local_label].get_label_group();
        assert(!label_group.empty());

        if (find(label_group.begin(),
                 label_group.end(),
                 label)
            == label_group.end()) {
            dump_label_mapping();
            cerr << "label " << label << " is not part of the "
                "local label it is mapped to" << endl;
            return false;
        }
    }

    for (size_t local_label = 0; local_label < local_label_infos.size(); ++local_label) {
        const LocalLabelInfo &local_label_info = local_label_infos[local_label];
        for (int label : local_label_info.get_label_group()) {
            if (label_to_local_label[label] != static_cast<int>(local_label)) {
                dump_label_mapping();
                cerr << "label " << label << " is not mapped "
                    "to the local label it is part of" << endl;
                return false;
            }
        }
    }
    return true;
}

void TransitionSystem::dump_label_mapping() const {
    utils::g_log << "to local label mapping: ";
    for (int label : labels) {
        utils::g_log << label << " -> "
                     << label_to_local_label[label] << ", ";
    }
    utils::g_log << endl;
    utils::g_log << "local to label mapping: ";
    for (size_t local_label = 0;
         local_label < local_label_infos.size(); ++local_label) {
        utils::g_log << local_label << ": "
                     << local_label_infos[local_label].get_label_group() << ", ";
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
    for (const LocalLabelInfo &local_label_info : *this) {
        total += local_label_info.get_transitions().size();
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

void TransitionSystem::dump_dot_graph(utils::LogProxy &log) const {
    assert(is_valid());
    if (log.is_at_least_debug()) {
        log << "digraph transition_system";
        for (size_t i = 0; i < incorporated_variables.size(); ++i)
            log << "_" << incorporated_variables[i];
        log << " {" << endl;
        log << "    node [shape = none] start;" << endl;
        for (int i = 0; i < num_states; ++i) {
            bool is_init = (i == init_state);
            bool is_goal = goal_states[i];
            log << "    node [shape = " << (is_goal ? "doublecircle" : "circle")
                << "] node" << i << ";" << endl;
            if (is_init)
                log << "    start -> node" << i << ";" << endl;
        }
        for (const LocalLabelInfo &local_label_info : *this) {
            const LabelGroup &label_group = local_label_info.get_label_group();
            const vector<Transition> &transitions = local_label_info.get_transitions();
            for (const Transition &transition : transitions) {
                int src = transition.src;
                int target = transition.target;
                log << "    node" << src << " -> node" << target << " [label = ";
                for (size_t i = 0; i < label_group.size(); ++i) {
                    if (i != 0)
                        log << "_";
                    log << "x" << label_group[i];
                }
                log << "];" << endl;
            }
        }
        log << "}" << endl;
    }
}

void TransitionSystem::dump_labels_and_transitions(utils::LogProxy &log) const {
    if (log.is_at_least_debug()) {
        log << tag() << "transitions" << endl;
        for (const LocalLabelInfo &local_label_info : *this) {
            const LabelGroup &label_group = local_label_info.get_label_group();
            log << "labels: " << label_group << endl;
            log << "transitions: ";
            const vector<Transition> &transitions = local_label_info.get_transitions();
            for (size_t i = 0; i < transitions.size(); ++i) {
                int src = transitions[i].src;
                int target = transitions[i].target;
                if (i != 0)
                    log << ",";
                log << src << " -> " << target;
            }
            utils::g_log << "cost: " << local_label_info.get_cost() << endl;
        }
    }
}

void TransitionSystem::statistics(utils::LogProxy &log) const {
    if (log.is_at_least_verbose()) {
        log << tag() << get_size() << " states, "
            << compute_total_transitions() << " arcs " << endl;
    }
}
}
