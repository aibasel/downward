#ifndef PRUNING_STUBBORN_SETS_SIMPLE_H
#define PRUNING_STUBBORN_SETS_SIMPLE_H

#include "stubborn_sets.h"

namespace stubborn_sets_simple {
/* Implementation of simple instantiation of strong stubborn sets.
   Disjunctive action landmarks are computed trivially.*/
class StubbornSetsSimple : public stubborn_sets::StubbornSets {
    /* interference_relation[op1_id] contains all operator ids
       of operators that interfere with op1. */
    std::vector<std::vector<ActionID>> interference_relation;

    void add_necessary_enabling_set(const FactPair &fact);
    void add_interfering(ActionID op_id);

    inline bool interfere(ActionID op1_id, ActionID op2_id) {
        return can_disable(op1_id, op2_id) ||
               can_conflict(op1_id, op2_id) ||
               can_disable(op2_id, op1_id);
    }
    void compute_interference_relation();
protected:
    virtual void initialize_stubborn_set(const State &state) override;
    virtual void handle_stubborn_operator(const State &state,
                                          ActionID op_id) override;
public:
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
};
}

#endif
