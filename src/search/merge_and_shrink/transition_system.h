#ifndef MERGE_AND_SHRINK_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_TRANSITION_SYSTEM_H

#include "types.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace utils {
enum class Verbosity;
}

namespace merge_and_shrink {
class Distances;
class GlobalLabels;

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

struct TransitionGroup {
    const LabelGroup &label_group;
    const std::vector<Transition> &transitions;
    int cost;
    TransitionGroup(const LabelGroup &label_group,
                    const std::vector<Transition> &transitions,
                    int cost)
        : label_group(label_group),
          transitions(transitions),
          cost(cost) {
    }
};

class TSConstIterator {
    /*
      This class allows users to easily iterate over both local labels and the
      global labels they represent, as well as their transitions.
    */
    const std::vector<LabelGroup> &local_to_global_labels;
    const std::vector<std::vector<Transition>> &local_label_to_transitions;
    const std::vector<int> &local_label_to_cost;
    int current_label;
public:
    TSConstIterator(
        const std::vector<LabelGroup> &local_to_global_labels,
        const std::vector<std::vector<Transition>> &local_label_to_transitions,
        const std::vector<int> &local_label_to_cost,
        bool end);
    void operator++();
    TransitionGroup operator*() const;

    bool operator==(const TSConstIterator &rhs) const {
        return current_label == rhs.current_label;
    }

    bool operator!=(const TSConstIterator &rhs) const {
        return current_label != rhs.current_label;
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

    const GlobalLabels &global_labels;
    /*
      All locally equivalent labels are grouped together, and their
      transitions are only stored once for every such group. Each such group
      is represented by a (virtual) local label. Local labels can be mapped
      back to the set of global labels they represent. Their cost is
      the minimum cost of all represented global labels.
    */
    std::vector<int> global_to_local_label;
    std::vector<LabelGroup> local_to_global_labels;
    std::vector<std::vector<Transition>> local_label_to_transitions;
    std::vector<int> local_label_to_cost;

    int num_states;
    std::vector<bool> goal_states;
    int init_state;

    void make_local_labels_contiguous();
    /*
      Check if two or more local labels are locally equivalent to each other,
      and if so, merge them and store their transitions only once.
    */
    void compute_locally_equivalent_labels();

    // Statistics and output
    int compute_total_transitions() const;
    std::string get_description() const;

    // Consistency
    /*
      The transitions for every group of locally equivalent labels are
      sorted (by source, by target) and there are no duplicates.
    */
    bool are_transitions_sorted_unique() const;
    /*
      The mapping global_to_local_label is consistent with the mapping
      local_to_global_labels.
    */
    bool is_label_mapping_consistent() const;
    void dump_label_mapping() const;
public:
    TransitionSystem(
        int num_variables,
        std::vector<int> &&incorporated_variables,
        const GlobalLabels &global_labels,
        std::vector<int> &&global_to_local_label,
        std::vector<LabelGroup> &&local_to_global_labels,
        std::vector<std::vector<Transition>> &&local_label_to_transitions,
        std::vector<int> &&local_label_to_cost,
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
        const GlobalLabels &global_labels,
        const TransitionSystem &ts1,
        const TransitionSystem &ts2,
        utils::Verbosity verbosity);

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
        utils::Verbosity verbosity);

    /*
      Applies the given label mapping, mapping old to new label numbers. This
      potentially means regrouping or adding new local labels.
    */
    void apply_label_reduction(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        bool only_equivalent_labels);

    TSConstIterator begin() const {
        return TSConstIterator(local_to_global_labels,
                               local_label_to_transitions,
                               local_label_to_cost,
                               false);
    }

    TSConstIterator end() const {
        return TSConstIterator(local_to_global_labels,
                               local_label_to_transitions,
                               local_label_to_cost,
                               true);
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
    void dump_dot_graph() const;
    void dump_labels_and_transitions() const;
    void statistics() const;

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
