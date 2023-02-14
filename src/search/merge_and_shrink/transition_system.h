#ifndef MERGE_AND_SHRINK_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_TRANSITION_SYSTEM_H

#include "types.h"

#include "../utils/collections.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class Distances;
class Labels;

struct Transition {
    int src;
    int target;

    Transition(int src, int target)
        : src(src), target(target) {
    }

    bool operator==(const Transition &other) const {
        return src == other.src && target == other.target;
    }

    bool operator<(const Transition &other) const {
        return src < other.src || (src == other.src && target < other.target);
    }

    // Required for "is_sorted_unique" in utilities
    bool operator>=(const Transition &other) const {
        return !(*this < other);
    }
};

using LabelGroup = std::vector<int>;

/*
  Class for representing groups of labels with equivalent transitions in a
  transition system. See also documentation for TransitionSystem.

  The local label is in a consistent state if label_group and transitions
  are sorted and unique.
*/
class LocalLabelInfo {
    // The sorted set of labels with identical transitions in a transition system.
    LabelGroup label_group;
    std::vector<Transition> transitions;
    // The cost is the minimum cost over all labels in label_group.
    int cost;
public:
    LocalLabelInfo(
        LabelGroup &&label_group,
        std::vector<Transition> &&transitions,
        int cost)
        : label_group(move(label_group)),
          transitions(move(transitions)),
          cost(cost) {
        assert(is_consistent());
    }

    void add_label(int label, int label_cost);

    /*
      Remove the labels in old_labels from label_group and add new_label to it.
      Requires old_labels to be sorted.
    */
    void apply_same_cost_label_mapping(
        int new_label, const std::vector<int> &old_labels);

    // Remove the labels in old_labels. Requires old_labels to be sorted.
    void remove_labels(const std::vector<int> &old_labels);

    void recompute_cost(const Labels &labels);
    void replace_transitions(std::vector<Transition> &&new_transitions);

    /*
      The given local label must have identical transitions. Its labels are
      moved into this local label info. The given local label is then
      invalidated.
    */
    void merge_local_label_info(LocalLabelInfo &local_label_info);

    // Empty all data structures.
    void deactivate();

    // A local label is active as long as it represents labels (in label_group).
    bool is_active() const {
        return !label_group.empty();
    }

    const LabelGroup &get_label_group() const {
        return label_group;
    }

    const std::vector<Transition> &get_transitions() const {
        return transitions;
    }

    int get_cost() const {
        return cost;
    }

    bool is_consistent() const;
};


/*
  Iterator class for TransitionSystem which provides access to the active
  entries of into local_label_infos.
*/
class TransitionSystemConstIterator {
    std::vector<LocalLabelInfo>::const_iterator it;
    std::vector<LocalLabelInfo>::const_iterator end_it;

    void advance_to_next_valid_index();
public:
    TransitionSystemConstIterator(
        std::vector<LocalLabelInfo>::const_iterator it,
        std::vector<LocalLabelInfo>::const_iterator end_it);
    TransitionSystemConstIterator &operator++();

    const LocalLabelInfo &operator*() const {
        return *it;
    }

    bool operator==(const TransitionSystemConstIterator &rhs) const {
        return it == rhs.it;
    }

    bool operator!=(const TransitionSystemConstIterator &rhs) const {
        return it != rhs.it;
    }
};


class TransitionSystem {
private:
    /*
      The following two attributes are only used for output.

      - num_variables: total number of variables in the factored
        transition system

      - incorporated_variables: variables that contributed to this
        transition system
    */
    const int num_variables;
    std::vector<int> incorporated_variables;

    const Labels &labels;
    /*
      All locally equivalent labels are grouped together, and their
      transitions are only stored once for every such group. Each such group
      is represented by a local label (LocalLabelInfo). Local labels can be
      mapped back to the set of labels they represent. Their cost is
      the minimum cost of all represented labels.
    */
    std::vector<int> label_to_local_label;
    std::vector<LocalLabelInfo> local_label_infos;

    int num_states;
    std::vector<bool> goal_states;
    int init_state;

    /*
      Check if two or more local labels are equivalent to each other,
      and if so, merge them and store their transitions only once.
    */
    void compute_equivalent_local_labels();

    // Statistics and output
    int compute_total_transitions() const;
    std::string get_description() const;

    /*
      The transitions for every group of locally equivalent labels are
      sorted (by source, by target) and there are no duplicates.
    */
    bool are_local_labels_consistent() const;

    /*
      The mapping label_to_local_label is consistent with local_label_infos.
    */
    bool is_label_mapping_consistent() const;
    void dump_label_mapping() const;
public:
    TransitionSystem(
        int num_variables,
        std::vector<int> &&incorporated_variables,
        const Labels &labels,
        std::vector<int> &&label_to_local_label,
        std::vector<LocalLabelInfo> &&local_label_infos,
        int num_states,
        std::vector<bool> &&goal_states,
        int init_state);
    TransitionSystem(const TransitionSystem &other);
    ~TransitionSystem();
    /*
      Factory method to construct the merge of two transition systems.

      Invariant: the children ts1 and ts2 must be solvable.
      (It is a bug to merge an unsolvable transition system.)
    */
    static std::unique_ptr<TransitionSystem> merge(
        const Labels &labels,
        const TransitionSystem &ts1,
        const TransitionSystem &ts2,
        utils::LogProxy &log);

    /*
      Applies the given state equivalence relation to the transition system.
      abstraction_mapping is a mapping from old states to new states, and it
      must be consistent with state_equivalence_relation in the sense that
      old states are only mapped to the same new state if they are in the same
      equivalence class as specified in state_equivalence_relation.
    */
    void apply_abstraction(
        const StateEquivalenceRelation &state_equivalence_relation,
        const std::vector<int> &abstraction_mapping,
        utils::LogProxy &log);

    /*
      Applies the given label mapping, mapping old to new label numbers. This
      potentially means regrouping or adding new local labels.
    */
    void apply_label_reduction(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        bool only_equivalent_labels);

    TransitionSystemConstIterator begin() const {
        return TransitionSystemConstIterator(local_label_infos.begin(), local_label_infos.end());
    }

    TransitionSystemConstIterator end() const {
        return TransitionSystemConstIterator(local_label_infos.end(), local_label_infos.end());
    }

    /*
      Method to identify the transition system in output.
      Print "Atomic transition system #x: " for atomic transition systems,
      where x is the variable. For composite transition systems, print
      "Transition system (x/y): " for a transition system containing x
      out of y variables.
    */
    std::string tag() const;

    bool is_valid() const;

    bool is_solvable(const Distances &distances) const;
    void dump_dot_graph(utils::LogProxy &log) const;
    void dump_labels_and_transitions(utils::LogProxy &log) const;
    void statistics(utils::LogProxy &log) const;

    int get_size() const {
        return num_states;
    }

    int get_init_state() const {
        return init_state;
    }

    bool is_goal_state(int state) const {
        return goal_states[state];
    }

    const std::vector<int> &get_incorporated_variables() const {
        return incorporated_variables;
    }
};
}

#endif
