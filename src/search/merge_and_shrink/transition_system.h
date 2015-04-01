#ifndef MERGE_AND_SHRINK_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_TRANSITION_SYSTEM_H

#include "../operator_cost.h"

#include <forward_list>
#include <iostream>
#include <list>
#include <string>
#include <tuple>
#include <vector>

class GlobalState;
class Label;
class Labels;

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

    bool operator!=(const Transition &other) const {
        return !(*this == other);
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
      and also the transitions.
    */
private:
    std::list<int> labels;
    std::vector<Transition> transitions;
    int cost;
public:
    explicit LabelGroup(std::vector<Transition> &&transitions_)
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
        return transitions;
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
        return transitions;
    }
    int get_cost() const {
        return cost;
    }
};

typedef std::list<LabelGroup>::iterator LabelGroupIter;
typedef std::list<LabelGroup>::const_iterator LabelGroupConstIter;

class TransitionSystem {
    // TODO: do we need these friend declarations? can we not make
    // the required private fields protected?
    friend class AtomicTransitionSystem;
    friend class CompositeTransitionSystem;

    static const int PRUNED_STATE = -1;
    static const int DISTANCE_UNKNOWN = -2;

    /*
      There should only be one instance of Labels at runtime. It is created
      and managed by MergeAndShrinkHeuristic. All transition system instances
      have a pointer to this object to ease access to the set of labels.
    */
    const Labels *labels;
    std::list<LabelGroup> grouped_labels;
    std::vector<std::tuple<LabelGroupIter, LabelIter> > label_to_positions;
    /*
      num_labels is always equal to labels->size(), with the exception during
      label reduction. Whenever new labels are generated through label
      reduction, this is updated immediately afterwards.
    */
    int num_labels;

    int num_states;

    std::vector<int> init_distances;
    std::vector<int> goal_distances;
    std::vector<bool> goal_states;
    AbstractStateRef init_state;

    int max_f;
    int max_g;
    int max_h;

    bool goal_relevant;

    mutable int peak_memory;

    /*
      Invariant of this class:
      A transition system is in a valid state iff:
       - The transitions for every group of locally equivalent labels are
         sorted (by source, by target) and there are no duplicates
         (are_transitions_sorted_unique() == true).
       - All labels are incorporated (is_label_reduced() == true)).
       - Distances are computed and stored (are_distances_computed() == true).
       - Locally equivalent labels are computed. This cannot explicitly be
         test because of labels and transitions being coupled in the data
         structure representing transitions.
      Note that those tests are expensive to compute and hence only used as
      an assertion.
    */
    bool is_valid() const;

    // Methods related to computation of distances
    void clear_distances();
    void compute_init_distances_unit_cost();
    void compute_goal_distances_unit_cost();
    void compute_init_distances_general_cost();
    void compute_goal_distances_general_cost();
    void discard_states(const std::vector<bool> &to_be_pruned_states);
    bool are_distances_computed() const;
    void compute_distances_and_prune();

    // Methods related to the representation of transitions and labels
    LabelGroupIter add_label_group(std::vector<Transition> &&transitions) {
        return grouped_labels.insert(grouped_labels.end(), LabelGroup(std::move(transitions)));
    }
    LabelGroupIter get_group_it(int label_no) {
        return std::get<0>(label_to_positions[label_no]);
    }
    LabelIter get_label_it(int label_no) {
        return std::get<1>(label_to_positions[label_no]);
    }
    void normalize_given_transitions(std::vector<Transition> &transitions) const;
    bool are_transitions_sorted_unique() const;
    bool is_label_reduced() const;
    void compute_locally_equivalent_labels();
    int total_transitions() const;
    int unique_unlabeled_transitions() const;
    virtual std::string description() const = 0;
protected:
    std::vector<int> varset;

    virtual AbstractStateRef get_abstract_state(const GlobalState &state) const = 0;
    virtual void apply_abstraction_to_lookup_table(
        const std::vector<AbstractStateRef> &abstraction_mapping) = 0;
    virtual int memory_estimate() const;
public:
    explicit TransitionSystem(Labels *labels);
    virtual ~TransitionSystem();

    static void build_atomic_transition_systems(std::vector<TransitionSystem *> &result,
                                                Labels *labels,
                                                OperatorCost cost_type);
    void apply_abstraction(std::vector<std::forward_list<AbstractStateRef> > &collapsed_groups);
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
    int get_cost(const GlobalState &state) const;
    void statistics(bool include_expensive_statistics) const;
    // NOTE: This will only return something useful if the memory estimates
    //       have been computed along the way by calls to statistics().
    // TODO: Find a better way of doing this that doesn't require
    //       a mutable attribute?
    int get_peak_memory_estimate() const;
    void dump_dot_graph() const;
    void dump_labels_and_transitions() const;
    int get_size() const {
        return num_states;
    }

    // Methods only used by shrink strategies.
    int get_max_f() const {
        return max_f;
    }
    int get_max_g() const { // currently not being used
        return max_g;
    }
    int get_max_h() const {
        return max_h;
    }
    bool is_goal_state(int state) const {
        return goal_states[state];
    }
    int get_init_distance(int state) const {
        return init_distances[state];
    }

    // Used by both shrink strategies and MergeDFP
    int get_goal_distance(int state) const {
        return goal_distances[state];
    }

    // Methods only used by MergeDFP.
    int get_num_labels() const {
        return num_labels;
    }
    bool is_goal_relevant() const {
        return goal_relevant;
    }
};


class AtomicTransitionSystem : public TransitionSystem {
    int variable;
    std::vector<AbstractStateRef> lookup_table;
protected:
    virtual void apply_abstraction_to_lookup_table(
        const std::vector<AbstractStateRef> &abstraction_mapping);
    virtual std::string description() const;
    virtual AbstractStateRef get_abstract_state(const GlobalState &state) const;
    virtual int memory_estimate() const;
public:
    AtomicTransitionSystem(Labels *labels, int variable);
    virtual ~AtomicTransitionSystem();
};


class CompositeTransitionSystem : public TransitionSystem {
    TransitionSystem *components[2];
    std::vector<std::vector<AbstractStateRef> > lookup_table;
protected:
    virtual void apply_abstraction_to_lookup_table(
        const std::vector<AbstractStateRef> &abstraction_mapping);
    virtual std::string description() const;
    virtual AbstractStateRef get_abstract_state(const GlobalState &state) const;
    virtual int memory_estimate() const;
public:
    CompositeTransitionSystem(Labels *labels, TransitionSystem *ts1, TransitionSystem *ts2);
    virtual ~CompositeTransitionSystem();
};

#endif
