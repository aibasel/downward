#ifndef PRUNING_STUBBORN_SETS_SIMPLE_H
#define PRUNING_STUBBORN_SETS_SIMPLE_H

#include "stubborn_sets.h"

namespace stubborn_sets_simple {
class StubbornSetsSimple : public stubborn_sets::StubbornSets {
    /* interference_relation[op1_no] contains all operator indices
       of operators that interfere with op1. */
    std::vector<std::vector<int>> interference_relation;

    void add_necessary_enabling_set(Fact fact);
    void add_interfering(OperatorProxy op);

    inline bool interfere(OperatorProxy op1, OperatorProxy op2) {
        return can_disable(op1, op2) ||
               can_conflict(op1, op2) ||
               can_disable(op2, op1);
    }
    void compute_interference_relation();
protected:
    virtual void initialize_stubborn_set(const State &state) override;
    virtual void handle_stubborn_operator(const State &state,
                                          OperatorProxy op) override;
public:
    StubbornSetsSimple();
    virtual ~StubbornSetsSimple() = default;
};
}

#endif
