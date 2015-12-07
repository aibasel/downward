#ifndef MERGE_AND_SHRINK_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_TRANSITION_SYSTEM_H

#include "types.h"

#include <forward_list>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class State;
class TaskProxy;
class Timer;


namespace MergeAndShrink {
class Distances;
class HeuristicRepresentation;
class LabelEquivalenceRelation;
class Labels;

struct Transition {
    int src;
    int target;

    Transition(int src_, int target_)
        : src(src_), target(target_) {
    }

    bool operator==(const Transition &other) const {
        return src == other.src && target == other.target;
    }

    bool operator<(const Transition &other) const {
        return src < other.src || (src == other.src && target < other.target);
    }

    bool operator>=(const Transition &other) const {
        return !(*this < other);
    }

    friend std::ostream &operator<<(std::ostream &os, const Transition &trans) {
        os << trans.src << "->" << trans.target;
        return os;
    }
};

class TransitionSystem;

class TSConstIterator {
    /*
      This class allows users to easily iterate over both label groups and
      their transitions of a TransitionSystem. Most importantly, it hides
      the data structure used by LabelEquivalenceRelation, which could be
      easily exchanged.
    */
    const LabelEquivalenceRelation &label_equivalence_relation;
    const std::vector<std::vector<Transition>> &transitions_by_group_id;
    // current is the actual iterator, representing the label group's id.
    int current;
public:
    TSConstIterator(const TransitionSystem &ts,
                    bool end);
    void operator++();
    bool operator==(const TSConstIterator &rhs) const {
        return current == rhs.current;
    }
    bool operator!=(const TSConstIterator &rhs) const {
        return current != rhs.current;
    }
    int get_id() const {
        return current;
    }
    int get_cost() const;
    LabelConstIter begin() const;
    LabelConstIter end() const;
    const std::vector<Transition> &get_transitions() const {
        return transitions_by_group_id[current];
    }
};

class TransitionSystem {
    friend class TSConstIterator;
public:
    static const int PRUNED_STATE;

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

    std::unique_ptr<LabelEquivalenceRelation> label_equivalence_relation;
    /*
      The transitions of a label group are indexed via its id. The id of a
      group does not change, and hence its transitions are never moved.
      Initially, every label is in a single label group, and its number is
      used to index transitions_of_groups. When adding new labels via label
      reduction, if a new label is not locally equivalent with any existing,
      we again use its number to index its transitions. When computing a
      composite, use the smallest label number of a group as index.

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

    bool goal_relevant; // TODO: Get rid of this?

    // Methods related to the representation of transitions and labels
    void normalize_given_transitions(std::vector<Transition> &transitions) const;
    void compute_locally_equivalent_labels();
    const std::vector<Transition> &get_transitions_for_group_id(int group_id) const {
        return transitions_by_group_id[group_id];
    }
    std::vector<Transition> &get_transitions_for_group_id(int group_id) {
        return transitions_by_group_id[group_id];
    }

    // Statistics and output
    int total_transitions() const;
    std::string description() const;
public:
    TransitionSystem(
        int num_variables,
        std::vector<int> &&incorporated_variables,
        std::unique_ptr<LabelEquivalenceRelation> &&label_equivalence_relation,
        std::vector<std::vector<Transition>> &&transitions_by_label,
        int num_states,
        std::vector<bool> &&goal_states,
        int init_state,
        bool goal_relevant,
        bool compute_label_equivalence_relation);
    ~TransitionSystem();
    /*
      Factory method to construct the merge of two transition systems.

      Invariant: the children ts1 and ts2 must be solvable.
      (It is a bug to merge an unsolvable transition system.)
    */
    static std::unique_ptr<TransitionSystem> merge(const Labels &labels,
                                                   const TransitionSystem &ts1,
                                                   const TransitionSystem &ts2);

    bool apply_abstraction(
        const StateEquivalenceRelation &state_equivalence_relation,
        const std::vector<int> &abstraction_mapping);
    void apply_label_reduction(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        bool only_equivalent_labels);

    TSConstIterator begin() const {
        return TSConstIterator(*this, false);
    }
    TSConstIterator end() const {
        return TSConstIterator(*this, true);
    }
    /*
      Method to identify the transition system in output.
      Print "Atomic transition system #x: " for atomic transition systems,
      where x is the variable. For composite transition systems, print
      "Transition system (x/y): " for the transition system containing x
      out of y variables.
    */
    std::string tag() const;
    /*
      The transitions for every group of locally equivalent labels are
      sorted (by source, by target) and there are no duplicates.
    */
    bool are_transitions_sorted_unique() const;
    bool is_solvable() const;
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
    bool is_goal_relevant() const {  // used by merge_dfp
        return goal_relevant;
    }
};
}

#endif
