#ifndef MERGE_AND_SHRINK_LABEL_REDUCER_H
#define MERGE_AND_SHRINK_LABEL_REDUCER_H

#include "../globals.h"
#include "../operator.h"
#include "../operator_cost.h"

#include <cassert>
#include <vector>

class OperatorSignature;

class LabelReducer {
    std::vector<const Operator *> reduced_label_by_index;
    inline int get_op_index(const Operator *op) const;

    int num_pruned_vars;
    int num_labels;
    int num_reduced_labels;

    OperatorSignature build_operator_signature(
        const Operator &op, OperatorCost cost_type,
        const vector<bool> &var_is_used) const;
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
