#ifndef MERGE_AND_SHRINK_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_TRANSITION_SYSTEM_H

#include <ext/slist>
#include <string>
#include <vector>

class EquivalenceRelation;
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
};

class TransitionSystem {
    friend class AtomicTransitionSystem;
    friend class CompositeTransitionSystem;

    friend class ShrinkStrategy; // for apply() -- TODO: refactor!

    static const int PRUNED_STATE = -1;
    static const int DISTANCE_UNKNOWN = -2;

    // There should only be one instance of Labels at runtime. It is created
    // and managed by MergeAndShrinkHeuristic. All transition system instances have
    // a copy of this object to ease access to the set of labels.
    const Labels *labels;
    /* num_labels equals to the number of labels that this transition system is
       "aware of", i.e. that have
       been incorporated into transitions_by_label. Whenever new labels are
       generated through label reduction, we do *not* update all transition systems
       immediately. This equals labels->size() after normalizing. */
    int num_labels;
    /* transitions_by_label and relevant_labels both have size of (2 * n) - 1
       if n is the number of operators, because when applying label reduction,
       at most n - 1 fresh labels can be generated in addition to the n
       original labels. */
    std::vector<std::vector<Transition> > transitions_by_label;
    std::vector<bool> relevant_labels;

    int num_states;

    std::vector<int> init_distances;
    std::vector<int> goal_distances;
    std::vector<bool> goal_states;
    AbstractStateRef init_state;

    int max_f;
    int max_g;
    int max_h;

    bool transitions_sorted_unique;
    bool goal_relevant;

    mutable int peak_memory;

    void clear_distances();
    void compute_init_distances_unit_cost();
    void compute_goal_distances_unit_cost();
    void compute_init_distances_general_cost();
    void compute_goal_distances_general_cost();
    void discard_states(const std::vector<bool> &to_be_pruned_states);

    // are_transitions_sorted_unique() is used to determine whether the
    // transitions of an transition system are sorted uniquely or not after
    // construction (composite transition system) and shrinking (apply_abstraction).
    bool are_transitions_sorted_unique() const;

    void apply_abstraction(std::vector<__gnu_cxx::slist<AbstractStateRef> > &collapsed_groups);

    int total_transitions() const;
    int unique_unlabeled_transitions() const;
protected:
    std::vector<int> varset;

    virtual AbstractStateRef get_abstract_state(const GlobalState &state) const = 0;
    virtual void apply_abstraction_to_lookup_table(
        const std::vector<AbstractStateRef> &abstraction_mapping) = 0;
    virtual int memory_estimate() const;
public:
    explicit TransitionSystem(Labels *labels);
    virtual ~TransitionSystem();

    // Two methods to identify the transition system in output.
    // tag is a convience method that upper-cases the first letter of
    // description and appends ": ";
    virtual std::string description() const = 0;
    std::string tag() const;

    static void build_atomic_transition_systems(std::vector<TransitionSystem *> &result,
                                                Labels *labels);

    int get_label_cost_by_index(int label_no) const;
    bool are_distances_computed() const;
    void compute_distances_and_prune();
    bool is_normalized() const;
    void normalize();
    EquivalenceRelation *compute_local_equivalence_relation() const;

    int get_cost(const GlobalState &state) const;
    void statistics(bool include_expensive_statistics) const;
    // NOTE: This will only return something useful if the memory estimates
    //       have been computed along the way by calls to statistics().
    // TODO: Find a better way of doing this that doesn't require
    //       a mutable attribute?
    int get_peak_memory_estimate() const;
    void release_memory();
    void dump_relevant_labels() const;
    void dump() const;

    bool is_solvable() const {
        return init_state != PRUNED_STATE;
    }
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
    int get_goal_distance(int state) const {
        return goal_distances[state];
    }
    const std::vector<Transition> &get_transitions_for_label(int label_no) const {
        return transitions_by_label[label_no];
    }
    int get_num_labels() const;

    // Methods only used by MergeDFP.
    void compute_label_ranks(std::vector<int> &label_ranks);
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
