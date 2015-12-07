#ifndef POR_STUBBORN_SETS_H
#define POR_STUBBORN_SETS_H

#include "../por_method.h"

namespace StubbornSets {

struct Fact {
    int var;
    int val;
    Fact(int v, int d) : var(v), val(d) {}
};

class StubbornSets : public PORMethod {
    long unpruned_successors_generated;
    long pruned_successors_generated;
        
protected:
    std::vector<std::vector<Fact> > op_preconds;
    std::vector<std::vector<Fact> > op_effects;
    
    // achievers[var][value] contains all operator indices of
    // operators that achieve the fact (var, value).
    std::vector<std::vector<std::vector<int> > > achievers;

    bool can_disable(int op1_no, int op2_no);
    bool can_conflict(int op1_no, int op2_no);
    bool interfere(int op1_no, int op2_no);

    void compute_sorted_operators();
    void compute_achievers();
    virtual void do_pruning(const GlobalState &state,
                            std::vector<const GlobalOperator *> &ops) = 0;
public:
    StubbornSets();
    virtual ~StubbornSets();

    virtual void prune_operators(const GlobalState &state,
                                 std::vector<const GlobalOperator *> &ops);
    virtual void dump_statistics() const;
};
}

#endif
