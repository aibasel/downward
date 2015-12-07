#ifndef POR_SIMPLE_STUBBORN_SETS_H
#define POR_SIMPLE_STUBBORN_SETS_H

#include "stubborn_sets.h"

namespace SimpleStubbornSets {

class SimpleStubbornSets : public StubbornSets::StubbornSets {

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
