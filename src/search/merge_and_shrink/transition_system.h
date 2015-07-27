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
class Labels;
class State;
class TaskProxy;
class Timer;

typedef int AbstractStateRef;

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

typedef std::list<int>::iterator LabelIter;
typedef std::list<int>::const_iterator LabelConstIter;

class LabelGroup {
    /*
      A label group contains a set of locally equivalent labels, possibly of
      different cost, stores the minimum cost of all labels of the group,
      and has a pointer to the position in TransitionSystem::transitions_of_groups
      where the transitions associated with this group live.
    */
    std::list<int> labels;
    std::vector<Transition> *transitions;
    int cost;
public:
    explicit LabelGroup(std::vector<Transition> *transitions_)
        : transitions(transitions_), cost(INF) {
    }
    void set_cost(int cost_) {
        cost = cost_;
    }
    LabelIter insert(int label) {
        return labels.insert(labels.end(), label);
    }
    void erase(LabelIter pos) {
        labels.erase(pos);
    }
    LabelIter begin() {
        return labels.begin();
    }
    LabelIter end() {
        return labels.end();
    }
    std::vector<Transition> &get_transitions() {
        return *transitions;
    }
    LabelConstIter end() const {
        return labels.end();
    }
    LabelConstIter begin() const {
        return labels.begin();
    }
    bool empty() const {
        return labels.empty();
    }
    int size() const {
        return labels.size();
    }
    const std::vector<Transition> &get_const_transitions() const {
        return *transitions;
    }
    int get_cost() const {
        return cost;
    }
};

typedef std::list<LabelGroup>::iterator LabelGroupIter;
typedef std::list<LabelGroup>::const_iterator LabelGroupConstIter;

class TransitionSystem {
public:
    static const int PRUNED_STATE = -1;

private:
    std::vector<int> var_id_set;

    /*
      There should only be one instance of Labels at runtime. It is created
      and managed by MergeAndShrinkHeuristic. All transition system instances
      have a pointer to this object to ease access to the set of labels.
    */
    const std::shared_ptr<Labels> labels;
    std::list<LabelGroup> grouped_labels;
    /*
      The transitions of a label group are never moved once they are stored
      at an index in transitions_of_groups.
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
    std::vector<std::vector<Transition> > transitions_of_groups;
    std::vector<std::pair<LabelGroupIter, LabelIter> > label_to_positions;

    // Number of variables of the task used by merge-and-shrink
    const int num_variables;

    int num_states;

    std::unique_ptr<HeuristicRepresentation> heuristic_representation;
    std::unique_ptr<Distances> distances;

    std::vector<bool> goal_states;
    AbstractStateRef init_state;

    bool goal_relevant;

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
    LabelGroupIter add_empty_label_group(std::vector<Transition> *transitions) {
        return grouped_labels.insert(grouped_labels.end(), LabelGroup(transitions));
    }
    void add_label_to_group(LabelGroupIter group_it, int label_no,
                            bool update_cost = true);
    int add_label_group(const std::vector<int> &new_labels);
    LabelGroupIter get_group_it(int label_no) {
        return label_to_positions[label_no].first;
    }
    LabelIter get_label_it(int label_no) {
        return label_to_positions[label_no].second;
    }
    void normalize_given_transitions(std::vector<Transition> &transitions) const;
    bool are_transitions_sorted_unique() const;
    void compute_locally_equivalent_labels();

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
        std::vector<std::vector<Transition> > &&transitions_by_label);

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

    const std::list<LabelGroup> &get_grouped_labels() const {
        return grouped_labels;
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
    void statistics(const Timer &timer,
                    bool include_expensive_statistics) const;
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
