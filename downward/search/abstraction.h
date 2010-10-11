#ifndef ABSTRACTION_H
#define ABSTRACTION_H

#include "mas_heuristic.h" // needed for ShrinkStrategy type;
                           // TODO: move that type somewhere else?

#include <ext/slist>
#include <vector>
using namespace std;
using namespace __gnu_cxx;

class State;
class Operator;

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
        return src != other.src || target != other.target;
    }

    bool operator<(const AbstractTransition &other) const {
        return src < other.src || (
                   src == other.src && target < other.target);
    }
};

class Abstraction {
    enum {QUITE_A_LOT = 1000000000};
    friend class AtomicAbstraction;
    friend class CompositeAbstraction;

    vector<const Operator *> relevant_operators;
    int num_states;
    vector<vector<AbstractTransition> > transitions_by_op;
    vector<int> init_distances;
    vector<int> goal_distances;
    AbstractStateRef init_state;

    int max_f;
    int max_g;
    int max_h;

    mutable int peak_memory;

    void compute_distances();
    void compute_init_distances();
    void compute_goal_distances();

    void partition_into_buckets(
        vector<vector<AbstractStateRef> > &buckets,
        ShrinkStrategy shrink_strategy) const;
    void compute_abstraction(
        vector<vector<AbstractStateRef> > &buckets, int target_size,
        vector<slist<AbstractStateRef> > &collapsed_groups) const;
    void compute_abstraction_dfp(
        int target_size,
        vector<slist<AbstractStateRef> > &collapsed_groups) const;
    void apply_abstraction(
        vector<slist<AbstractStateRef> > &collapsed_groups);

    int total_transitions() const;
    int unique_unlabeled_transitions() const;

    void normalize(bool simplify_labels);
protected:
    enum {INVALID = -2};

    vector<int> varset;

    virtual AbstractStateRef get_abstract_state(const State &state) const = 0;
    virtual void apply_abstraction_to_lookup_table(
        const vector<AbstractStateRef> &abstraction_mapping) = 0;
    virtual int memory_estimate() const;
public:
    Abstraction();
    virtual ~Abstraction();

    static void build_atomic_abstractions(vector<Abstraction *> &result);
    bool is_solvable() const;

    int get_cost(const State &state) const;
    int size() const;
    void statistics(bool include_expensive_statistics) const;

    int get_peak_memory_estimate() const;
    // NOTE: This will only return something useful if the memory estimates
    //       have been computed along the way by calls to statistics().
    // TODO: Find a better way of doing this that doesn't require
    //       a mutable attribute?

    void shrink(int threshold, ShrinkStrategy shrink_strategy,
                bool force = false);
    void release_memory();

    void dump() const;
};

class AtomicAbstraction : public Abstraction {
    int variable;
    vector<AbstractStateRef> lookup_table;
protected:
    virtual void apply_abstraction_to_lookup_table(
        const vector<AbstractStateRef> &abstraction_mapping);
    virtual AbstractStateRef get_abstract_state(const State &state) const;
    virtual int memory_estimate() const;
public:
    AtomicAbstraction(int variable_);
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
    CompositeAbstraction(Abstraction *abs1, Abstraction *abs2,
                         bool simplify_labels);
};

#endif
