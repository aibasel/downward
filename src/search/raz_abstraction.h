#ifndef ABSTRACTION_H
#define ABSTRACTION_H

#include "operator_cost.h"
#include "shrink_strategy.h"

#include <ext/slist>
#include <vector>
using namespace std;
using namespace __gnu_cxx;

class State;
class Operator;

struct AbstractTransition {
    AbstractStateRef src;
    AbstractStateRef target;
    int cost;

    AbstractTransition(AbstractStateRef src_, AbstractStateRef target_,
                       int cost_)
        : src(src_), target(target_), cost(cost_) {
    }

    bool operator==(const AbstractTransition &other) const {
        return src == other.src && target == other.target && cost == other.cost;
    }

    bool operator!=(const AbstractTransition &other) const {
        return src != other.src || target != other.target || cost != other.cost;
    }

    bool operator<(const AbstractTransition &other) const {
        return src < other.src || (src == other.src && target < other.target)
               || (src == other.src && target == other.target && cost
                   < other.cost);
    }
};

class Abstraction {
    friend class AtomicAbstraction;
    friend class CompositeAbstraction;

    friend class ShrinkStrategy; // for apply() -- TODO: refactor!

    const bool is_unit_cost;
    const OperatorCost cost_type;

    ShrinkStrategy *shrink_strategy;

    vector<const Operator *> relevant_operators;
    int num_states;
    vector<vector<AbstractTransition> > transitions_by_op;

    vector<int> init_distances;
    vector<int> goal_distances;
    vector<bool> goal_states;
    AbstractStateRef init_state;

    int max_f;
    int max_g;
    int max_h;

    mutable int peak_memory;

    void compute_distances();
    void compute_init_distances_unit_cost();
    void compute_goal_distances_unit_cost();
    void compute_init_distances_general_cost();
    void compute_goal_distances_general_cost();

    void apply_abstraction(vector<slist<AbstractStateRef> > &collapsed_groups);

    int total_transitions() const;
    int unique_unlabeled_transitions() const;

    void normalize(bool use_label_reduction);
protected:
    enum {
        INVALID = -2
    };

    vector<int> varset;

    virtual AbstractStateRef get_abstract_state(const State &state) const = 0;
    virtual void apply_abstraction_to_lookup_table(const vector<
                                                       AbstractStateRef> &abstraction_mapping) = 0;
    virtual int memory_estimate() const;
public:
    Abstraction(bool is_unit_cost, OperatorCost cost_type);
    virtual ~Abstraction();

    static void build_atomic_abstractions(
            bool is_unit_cost, OperatorCost cost_type,
            std::vector<Abstraction *> &result);
    bool is_solvable() const;

    int get_cost(const State &state) const;
    int size() const;
    void statistics(bool include_expensive_statistics) const;

    int get_peak_memory_estimate() const;
    // NOTE: This will only return something useful if the memory estimates
    //       have been computed along the way by calls to statistics().
    // TODO: Find a better way of doing this that doesn't require
    //       a mutable attribute?

    int unique_unlabeled_transitions(const vector<int> &relevant_ops) const;
    bool is_in_varset(int var) const;

    void release_memory();

    void dump() const;

    // The following methods exist for the benefit of shrink strategies.
    int get_max_f() const;
    int get_max_g() const;
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

    int get_num_ops() const {
        return transitions_by_op.size();
    }

    const std::vector<AbstractTransition> &get_transitions_for_op(int op_no) {
        return transitions_by_op[op_no];
    }
};

class AtomicAbstraction : public Abstraction {
    int variable;
    vector<AbstractStateRef> lookup_table;
protected:
    virtual void apply_abstraction_to_lookup_table(const vector<
                                                       AbstractStateRef> &abstraction_mapping);
    virtual AbstractStateRef get_abstract_state(const State &state) const;
    virtual int memory_estimate() const;
public:
    AtomicAbstraction(bool is_unit_cost, OperatorCost cost_type, int variable);
    virtual ~AtomicAbstraction();
};

class CompositeAbstraction : public Abstraction {
    Abstraction *components[2];
    vector<vector<AbstractStateRef> > lookup_table;
protected:
    virtual void apply_abstraction_to_lookup_table(
        const vector<AbstractStateRef> &abstraction_mapping);
    virtual AbstractStateRef get_abstract_state(const State &state) const;
    virtual int memory_estimate() const;
public:
    CompositeAbstraction(
        bool is_unit_cost, OperatorCost cost_type,
        Abstraction *abs1, Abstraction *abs2,
        bool use_label_reduction,
        ShrinkStrategy::WhenToNormalize when_to_normalize);
    virtual ~CompositeAbstraction();
};

#endif
