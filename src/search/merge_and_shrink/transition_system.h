#ifndef MERGE_AND_SHRINK_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_TRANSITION_SYSTEM_H

#include "types.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace merge_and_shrink {
class Distances;
class LabelEquivalenceRelation;
class LabelGroup;
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

struct GroupAndTransitions {
    const LabelGroup &label_group;
    const std::vector<Transition> &transitions;
    GroupAndTransitions(const LabelGroup &label_group,
                        const std::vector<Transition> &transitions)
        : label_group(label_group),
          transitions(transitions) {
    }
};

class TSConstIterator {
    /*
      This class allows users to easily iterate over both label groups and
      their transitions of a TransitionSystem. Most importantly, it hides
      the data structure used by LabelEquivalenceRelation, which could be
      easily exchanged.
    */
    const LabelEquivalenceRelation &label_equivalence_relation;
    const std::vector<std::vector<Transition>> &transitions_by_group_id;
    // current_group_id is the actual iterator
    int current_group_id;

    void next_valid_index();
public:
    TSConstIterator(const LabelEquivalenceRelation &label_equivalence_relation,
                    const std::vector<std::vector<Transition>> &transitions_by_group_id,
                    bool end);
    void operator++();
    GroupAndTransitions operator*() const;

    bool operator==(const TSConstIterator &rhs) const {
        return current_group_id == rhs.current_group_id;
    }

    bool operator!=(const TSConstIterator &rhs) const {
        return current_group_id != rhs.current_group_id;
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

    /*
      All locally equivalent labels are grouped together, and their
      transitions are only stored once for every such group, see below.

      LabelEquivalenceRelation stores the equivalence relation over all
      labels of the underlying factored transition system.
    */
    std::unique_ptr<LabelEquivalenceRelation> label_equivalence_relation;

    /*
      The transitions of a label group are indexed via its ID. The ID of a
      group does not change, and hence its transitions are never moved.

      We tested different alternatives to store the transitions, but they all
      performed worse: storing a vector transitions in the label group increases
      memory usage and runtime; storing the transitions more compactly and
      incrementally increasing the size of transitions_of_groups whenever a
      new label group is added also increases runtime. See also issue492 and
      issue521.
    */
    std::vector<std::vector<Transition>> transitions_by_group_id;

    int num_states;
    std::vector<bool> goal_states;
    int init_state;

    /*
      Check if two or more labels are locally equivalent to each other, and
      if so, update the label equivalence relation.
    */
    void compute_locally_equivalent_labels();

    const std::vector<Transition> &get_transitions_for_group_id(int group_id) const {
        return transitions_by_group_id[group_id];
    }

    // Statistics and output
    int compute_total_transitions() const;
    std::string get_description() const;
public:
    TransitionSystem(
        int num_variables,
        std::vector<int> &&incorporated_variables,
        std::unique_ptr<LabelEquivalenceRelation> &&label_equivalence_relation,
        std::vector<std::vector<Transition>> &&transitions_by_label,
        int num_states,
        std::vector<bool> &&goal_states,
        int init_state,
        bool compute_label_equivalence_relation);
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
        Verbosity verbosity);

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
        Verbosity verbosity);

    /*
      Applies the given label mapping, mapping old to new label numbers. This
      updates the label equivalence relation which is internally used to group
      locally equivalent labels and store their transitions only once.
    */
    void apply_label_reduction(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        bool only_equivalent_labels);

    TSConstIterator begin() const {
        return TSConstIterator(*label_equivalence_relation,
                               transitions_by_group_id,
                               false);
    }

    TSConstIterator end() const {
        return TSConstIterator(*label_equivalence_relation,
                               transitions_by_group_id,
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

    /*
      The transitions for every group of locally equivalent labels are
      sorted (by source, by target) and there are no duplicates.
    */
    bool are_transitions_sorted_unique() const;

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
