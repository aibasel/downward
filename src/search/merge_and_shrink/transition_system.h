#ifndef MERGE_AND_SHRINK_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_TRANSITION_SYSTEM_H

#include <forward_list>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Distances;
class HeuristicRepresentation;
class LabelEquivalenceRelation;
class Labels;
class State;
class TaskProxy;
class Timer;

typedef int AbstractStateRef;
// Duplicate from LabelEquivalenceRelation
typedef std::list<int>::const_iterator LabelConstIter;

// Positive infinity. The name "INFINITY" is taken by an ISO C99 macro.
extern const int INF;

struct Transition {
    AbstractStateRef src;
    AbstractStateRef target;

    Transition(AbstractStateRef src_, AbstractStateRef target_)
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

class TSConstIterator {
    /*
      This class allows users to easily iterate over both label groups and
      their transitions of a TransitionSystem. Most importantly, it hides
      the data structure used by LabelEquivalenceRelation, which could be
      easily exchanged.
    */
    const std::shared_ptr<LabelEquivalenceRelation> label_equivalence_relation;
    // TODO: when we move more data out of TransitionSystem, such as
    // Distances, HeuristicRepresentation and LabelEquivalenceRelation,
    // we could easily change the following to be the TransitionSystem, rather
    // than the underlying data structure. Then we could also move this
    // iterator class in its own file.
    const std::vector<std::vector<Transition> > &transitions_by_group_id;
    // current is the actual iterator, representing the label group's id.
    int current;
public:
    TSConstIterator(const std::shared_ptr<LabelEquivalenceRelation> label_equivalence_relation,
                    const std::vector<std::vector<Transition> > &transitions_by_group_id,
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
public:
    static const int PRUNED_STATE = -1;

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

    std::shared_ptr<LabelEquivalenceRelation> label_equivalence_relation;
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
    std::vector<std::vector<Transition> > transitions_by_group_id;

    int num_states;

    std::unique_ptr<HeuristicRepresentation> heuristic_representation;
    std::unique_ptr<Distances> distances;

    std::vector<bool> goal_states;
    AbstractStateRef init_state;

    bool goal_relevant; // TODO: Get rid of this?

    /*
      Invariant of this class:
      A transition system is in a valid state iff:
       - The transitions for every group of locally equivalent labels are
         sorted (by source, by target) and there are no duplicates
         (are_transitions_sorted_unique() == true).
       - Distances are computed and stored (are_distances_computed() == true).
       - Locally equivalent labels are computed. This cannot explicitly be
         tested because of labels and transitions being coupled in the data
         structure representing transitions.
      Note that those tests are expensive to compute and hence only used as
      an assertion.
    */
    bool is_valid() const;

    void compute_distances_and_prune();
    void discard_states(const std::vector<bool> &to_be_pruned_states);

    // Methods related to the representation of transitions and labels
    void normalize_given_transitions(std::vector<Transition> &transitions) const;
    bool are_transitions_sorted_unique() const;
    void compute_locally_equivalent_labels();
    const std::vector<Transition> &get_transitions_for_group_id(int group_id) const {
        return transitions_by_group_id[group_id];
    }
    std::vector<Transition> &get_transitions_for_group_id(int group_id) {
        return transitions_by_group_id[group_id];
    }

    // Statistics and output
    int total_transitions() const;
    int unique_unlabeled_transitions() const;
    std::string description() const;

    TransitionSystem(const TaskProxy &task_proxy,
                     const std::shared_ptr<Labels> labels);
public:
    // Constructor for an atomic transition system.
    TransitionSystem(
        const TaskProxy &task_proxy,
        const std::shared_ptr<Labels> labels,
        int var_id,
        std::vector<std::vector<Transition> > && transitions_by_label);

    /*
      Constructor that merges two transition systems.

      Invariant: the children ts1 and ts2 must be solvable.
      (It is a bug to merge an unsolvable transition system.)
    */
    TransitionSystem(const TaskProxy &task_proxy,
                     const std::shared_ptr<Labels> labels,
                     TransitionSystem *ts1,
                     TransitionSystem *ts2);
    ~TransitionSystem();

    bool apply_abstraction(const std::vector<std::forward_list<AbstractStateRef> > &collapsed_groups);
    void apply_label_reduction(const std::vector<std::pair<int, std::vector<int> > > &label_mapping,
                               bool only_equivalent_labels);
    void release_memory();

    TSConstIterator begin() const {
        return TSConstIterator(
                   label_equivalence_relation, transitions_by_group_id, false);
    }
    TSConstIterator end() const {
        return TSConstIterator(
                   label_equivalence_relation, transitions_by_group_id, true);
    }
    /*
      Method to identify the transition system in output.
      Print "Atomic transition system #x: " for atomic transition systems,
      where x is the variable. For composite transition systems, print
      "Transition system (x/y): " for the transition system containing x
      out of y variables.
    */
    std::string tag() const;
    bool is_solvable() const;
    int get_cost(const State &state) const;
    void statistics(const Timer &timer) const;
    void dump_dot_graph() const;
    void dump_labels_and_transitions() const;
    int get_size() const {
        return num_states;
    }

    int get_init_state() const {
        return init_state;
    }

    bool is_goal_state(int state) const {
        return goal_states[state];
    }

    /*
      TODO: We probably want to get rid of the methods below that just
      forward to distances, by giving the users of these methods
      access to the the distances object instead.

      This might also help address a possible performance problem we
      might have at the moment, now that these methods are no longer
      inlined here. (To be able to inline them, we would need to
      include distances.h here, which we would rather not.)
    */
    int get_max_f() const; // used by shrink strategies
    int get_max_g() const; // unused
    int get_max_h() const; // used by shrink strategies
    int get_init_distance(int state) const; // used by shrink_fh
    int get_goal_distance(int state) const; // used by shrink strategies and merge_dfp

    int get_num_labels() const;      // used by merge_dfp
    bool is_goal_relevant() const {  // used by merge_dfp
        return goal_relevant;
    }
};

#endif
