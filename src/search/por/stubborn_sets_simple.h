#ifndef POR_STUBBORN_SETS_SIMPLE_H
#define POR_STUBBORN_SETS_SIMPLE_H

#include "stubborn_sets.h"

namespace StubbornSetsSimple {
class StubbornSetsSimple : public StubbornSets::StubbornSets {
    /* interference_relation[op1_no] contains all operator indices
       of operators that interfere with op1. */
    std::vector<std::vector<int>> interference_relation;

    void mark_as_stubborn(int op_no);
    void add_necessary_enabling_set(std::pair<int, int> fact);
    void add_interfering(int op_no);

    void compute_interference_relation();
protected:
    virtual void compute_stubborn_set(const GlobalState &state,
				      std::vector<const GlobalOperator *> &ops);
    virtual void initialize();
};
}

#endif
