#ifndef MERGE_AND_SHRINK_ABSTRACTION_H
#define MERGE_AND_SHRINK_ABSTRACTION_H

#include <ext/slist>
#include <string>
#include <vector>

class EquivalenceRelation;
class Label;
class Labels;
class State;

typedef int AbstractStateRef;

struct AbstractTransition {
    AbstractStateRef src;
    AbstractStateRef target;

    AbstractTransition(AbstractStateRef src_, AbstractStateRef target_)
        : src(src_), target(target_) {
    }

    bool operator==(const AbstractTransition &other) const {
        return src == other.src && target == other.target;
    }

    bool operator!=(const AbstractTransition &other) const {
        return !(*this == other);
    }

    bool operator<(const AbstractTransition &other) const {
        return src < other.src || (src == other.src && target < other.target);
    }

    bool operator>=(const AbstractTransition &other) const {
        return !(*this < other);
    }
};

class Abstraction {
    friend class AtomicAbstraction;
    friend class CompositeAbstraction;

    friend class ShrinkStrategy; // for apply() -- TODO: refactor!

    static const int PRUNED_STATE = -1;
    static const int DISTANCE_UNKNOWN = -2;

    // There should only be one instance of Labels at runtime. It is created
    // and managed by MergeAndShrinkHeuristic. All abstraction instances have
    // a copy of this object to ease access to the set of labels.
    const Labels *labels;
    /* num_labels equals to the number of labels that this abstraction is
       "aware of", i.e. that have
       been incorporated into transitions_by_label. Whenever new labels are
       generated through label reduction, we do *not* update all abstractions
       immediately. This equals labels->size() after normalizing. */
    int num_labels;
    /* transitions_by_label and relevant_labels both have size of (2 * n) - 1
       if n is the number of operators, because when applying label reduction,
       at most n - 1 fresh labels can be generated in addition to the n
       original labels. */
    std::vector<std::vector<AbstractTransition> > transitions_by_label;
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

    // are_transitions_sorted_unique() is used to determine whether the
    // transitions of an abstraction are sorted uniquely or not after
    // construction (composite abstraction) and shrinking (apply_abstraction).
    bool are_transitions_sorted_unique() const;

    void apply_abstraction(std::vector<__gnu_cxx::slist<AbstractStateRef> > &collapsed_groups);

    int total_transitions() const;
    int unique_unlabeled_transitions() const;
protected:
    std::vector<int> varset;

    virtual AbstractStateRef get_abstract_state(const State &state) const = 0;
    virtual void apply_abstraction_to_lookup_table(const std::vector<
                                                       AbstractStateRef> &abstraction_mapping) = 0;
    virtual int memory_estimate() const;
public:
    Abstraction(Labels *labels);
    virtual ~Abstraction();

    // Two methods to identify the abstraction in output.
    // tag is a convience method that upper-cases the first letter of
    // description and appends ": ";
    virtual std::string description() const = 0;
    std::string tag() const;

    static void build_atomic_abstractions(std::vector<Abstraction *> &result,
                                          Labels *labels);
    bool is_solvable() const;

    int get_cost(const State &state) const;
    int size() const;
    void statistics(bool include_expensive_statistics) const;

    int get_peak_memory_estimate() const;
    // NOTE: This will only return something useful if the memory estimates
    //       have been computed along the way by calls to statistics().
    // TODO: Find a better way of doing this that doesn't require
    //       a mutable attribute?

    bool are_distances_computed() const;
    void compute_distances();
    bool is_normalized() const;
    void normalize();
    EquivalenceRelation *compute_local_equivalence_relation() const;
    void release_memory();

    void dump_relevant_labels() const;
    void dump() const;

    // The following methods exist for the benefit of shrink strategies.
    int get_max_f() const;
    int get_max_g() const; // Not being used!
    int get_max_h() const;

    bool is_goal_state(int state) const {
        return goal_states[state];
    }

    int get_init_distance(int state) const {
        return init_distances[state];
    }

    int get_goal_distance(int state) const {
        return goal_distances[state];
    }

    // These methods should be private but is public for shrink_bisimulation
    int get_label_cost_by_index(int label_no) const;
    const std::vector<AbstractTransition> &get_transitions_for_label(int label_no) const;
    // This method is shrink_bisimulation-exclusive
    int get_num_labels() const;
    // These methods are used by non_linear_merge_strategy
    void compute_label_ranks(std::vector<int> &label_ranks);
    bool is_goal_relevant() const {
        return goal_relevant;
    }
    // This is used by the "old label reduction" method
    const std::vector<int> &get_varset() const {
        return varset;
    }
};

class AtomicAbstraction : public Abstraction {
    int variable;
    std::vector<AbstractStateRef> lookup_table;
protected:
    virtual std::string description() const;
    virtual void apply_abstraction_to_lookup_table(
        const std::vector<AbstractStateRef> &abstraction_mapping);
    virtual AbstractStateRef get_abstract_state(const State &state) const;
    virtual int memory_estimate() const;
public:
    AtomicAbstraction(Labels *labels, int variable);
    virtual ~AtomicAbstraction();
};

class CompositeAbstraction : public Abstraction {
    Abstraction *components[2];
    std::vector<std::vector<AbstractStateRef> > lookup_table;
protected:
    virtual std::string description() const;
    virtual void apply_abstraction_to_lookup_table(
        const std::vector<AbstractStateRef> &abstraction_mapping);
    virtual AbstractStateRef get_abstract_state(const State &state) const;
    virtual int memory_estimate() const;
public:
    CompositeAbstraction(Labels *labels, Abstraction *abs1, Abstraction *abs2);
    virtual ~CompositeAbstraction();
};

#endif
