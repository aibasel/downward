#ifndef OPERATOR_REGISTRY_H
#define OPERATOR_REGISTRY_H

/* TODO: Rename source files to label_reducer.{h,cc}. Only do this
         after main changes are merged and tested, since reviewing
         changes across renames is more difficult. */

#include "globals.h"
#include "operator_cost.h"

#include <cassert>
#include <vector>

class Operator;

class LabelReducer {
    std::vector<const Operator *> reduced_label_by_index;
    inline int get_op_index(const Operator *op) const;

    int num_pruned_vars;
    int num_labels;
    int num_reduced_labels;
public:
    LabelReducer(
        const std::vector<const Operator *> &relevant_operators,
        const std::vector<int> &pruned_vars,
        OperatorCost cost_type);
    ~LabelReducer();
    inline const Operator *get_reduced_label(const Operator *op) const;
    void statistics() const;
};

inline int LabelReducer::get_op_index(const Operator *op) const {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < g_operators.size());
    return op_index;
}

inline const Operator *LabelReducer::get_reduced_label(
    const Operator *op) const {
    const Operator *reduced_label = reduced_label_by_index[get_op_index(op)];
    assert(reduced_label);
    return reduced_label;
}

#endif
