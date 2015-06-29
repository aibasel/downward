#ifndef POR_SIMPLE_STUBBORN_SETS_H
#define POR_SIMPLE_STUBBORN_SETS_H

#include "por_method.h"
#include "../globals.h"
#include "../global_operator.h"
#include "../successor_generator.h"
#include <vector>
#include <cassert>
#include <algorithm>

class GlobalState;

namespace POR {
struct Fact {
    int var;
    int val;
    Fact(int v, int d) : var(v), val(d) {}
};

inline bool operator<(const Fact &lhs, const Fact &rhs) {
    return lhs.var < rhs.var;
}
////////////////////////////////////////////

class SimpleStubbornSets : public PORMethodWithStatistics {
    // achievers[var][value] contains all operator indices of
    // operators that achieve the fact (var, value).
    std::vector<std::vector<std::vector<int> > > achievers;

    // interference_relation[op1_no] contains all operator indices
    // of operators that interfere with op1.
    std::vector<std::vector<int> > interference_relation;

    // stubborn[op_no] is true iff the operator with operator index
    // op_no is contained in the stubborn set
    std::vector<bool> stubborn;

    // stubborn_queue contains the operator indices of operators that
    // have been marked and stubborn but have not yet been processed
    // (i.e. more operators might need to be added to stubborn because
    // of the operators in the queue).
    std::vector<int> stubborn_queue;

    void mark_as_stubborn(int op_no);
    void add_nes_for_fact(std::pair<int, int> fact);
    void add_interfering(int op_no);

    void compute_interference_relation();
    void compute_achievers();
    void compute_sorted_operators();
protected:
    virtual void do_pruning(const GlobalState &state,
                            std::vector<const GlobalOperator *> &ops);
public:
    SimpleStubbornSets();
    ~SimpleStubbornSets();

    virtual void dump_options() const;
};
}



#endif
