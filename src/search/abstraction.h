#ifndef ABSTRACTION_H
#define ABSTRACTION_H

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

struct AbstractTargetOp {
    AbstractStateRef target;
    int op;

    AbstractTargetOp(AbstractStateRef target_, AbstractStateRef op_)
        : target(target_), op(op_) {
    }

    bool operator==(const AbstractTargetOp &other) const {
        return target == other.target && op == other.op;
    }

    bool operator!=(const AbstractTargetOp &other) const {
        return target != other.target || op != other.op;
    }

    bool operator<(const AbstractTargetOp &other) const {
        return target < other.target || (target == other.target && op
                                         < other.op);
    }
};

class Abstraction {
    friend class ShrinkFH;
    friend class ShrinkDFP;
    friend class ShrinkBisimulation;
    enum {
        QUITE_A_LOT = 1000000000
    };
    friend class AtomicAbstraction;
    friend class CompositeAbstraction;

    ShrinkStrategy *shrink_strategy;

    vector<const Operator *> relevant_operators;
    int num_states;
    vector<vector<AbstractTransition> > transitions_by_op;

    vector<vector<AbstractTargetOp> > transitions_by_source;

    vector<int> init_distances;
    vector<int> goal_distances;
    vector<bool> goal_states;
    AbstractStateRef init_state;

    int max_f;
    int max_g;
    int max_h;

    mutable int peak_memory;

    void compute_distances();
    void compute_init_distances();
    void compute_goal_distances();

    void apply_abstraction(vector<slist<AbstractStateRef> > &collapsed_groups);

    int total_transitions() const;
    int unique_unlabeled_transitions() const;

    void add_relevant_reducible_op_pairs(
        const vector<pair<int, int> > &succ_sig1, const vector<pair<int,
                                                                    int> > &succ_sig2, vector<pair<int, int> > &pairs) const;
    //	bool are_greedy_bisimilar(const vector<pair<int, int> > &succ_sig1,
    //			const vector<pair<int, int> > &succ_sig2,
    //			const vector<int> &group_to_h, int source_group_h) const;
    bool are_bisimilar_wrt_label_reduction(
        const vector<pair<int, int> > &succ_sig1, const vector<pair<int,
                                                                    int> > &succ_sig2,
        const vector<pair<int, int> > &pairs_of_labels_to_reduce) const;
    bool are_bisimilar(const vector<pair<int, int> > &succ_sig1, const vector<
                           pair<int, int> > &succ_sig2, bool ignore_all_labels,
                       bool greedy_bisim, bool further_label_reduction,
                       const vector<int> &group_to_h, int source_h_1, int source_h_2,
                       const vector<pair<int, int> > &pairs_of_labels_to_reduce) const;
    void normalize(bool simplify_labels);
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

    int unique_unlabeled_transitions(const vector<int> &relevant_ops) const;
    bool is_in_varset(int var) const;

    void shrink(int threshold, bool force = false);
    //    void reduce_operators(int op1, int op2);
    void release_memory();

    void dump() const;

    void dump_transitions_by_src() const;

    void set_shrink_strategy(ShrinkStrategy *s) {
        delete shrink_strategy;  //is this wise?
        shrink_strategy = s;
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
    AtomicAbstraction(int variable_);
};

class CompositeAbstraction : public Abstraction {
    Abstraction *components[2];
    vector<vector<AbstractStateRef> > lookup_table;
protected:
    virtual void apply_abstraction_to_lookup_table(const vector<
                                                       AbstractStateRef> &abstraction_mapping);
    virtual AbstractStateRef get_abstract_state(const State &state) const;
    virtual int memory_estimate() const;
public:
    CompositeAbstraction(Abstraction *abs1, Abstraction *abs2,
                         bool simplify_labels, bool normalize_after_composition);
};


typedef vector<pair<int, int> > SuccessorSignature;

struct Signature {
    int h;
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h_, int group_, const SuccessorSignature &succ_signature_,
              int state_)
        : h(h_), group(group_), succ_signature(succ_signature_), state(state_) {
    }

    bool matches(const Signature &other) const {
        return h == other.h && group == other.group && succ_signature
               == other.succ_signature;
    }

    bool operator<(const Signature &other) const {
        if (h != other.h)
            return h < other.h;
        if (group != other.group)
            return group < other.group;
        if (succ_signature != other.succ_signature)
            return succ_signature < other.succ_signature;
        return state < other.state;
    }
};


#endif
