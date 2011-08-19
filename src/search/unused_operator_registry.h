#ifndef OPERATOR_REGISTRY_H
#define OPERATOR_REGISTRY_H

#include "globals.h"
#include "operator_cost.h"

#include <vector>

class Operator;
class OperatorSignature;

class OperatorRegistry {

    std::vector<const Operator *> canonical_operators;
    inline int get_op_index(const Operator *op) const;

    int num_vars;
    int num_operators;
    int num_canonical_operators;

    OperatorSignature build_operator_signature(
        const Operator &op, OperatorCost cost_type,
        const vector<bool> &var_is_used) const;
public:
    OperatorRegistry(
        const std::vector<const Operator *> &relevant_operators,
        const std::vector<int> &pruned_vars,
        OperatorCost cost_type);
    ~OperatorRegistry();
    inline const Operator *get_canonical_operator(
        const Operator *) const;
    void statistics() const;
};

inline int OperatorRegistry::get_op_index(const Operator *op) const {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < g_operators.size());
    return op_index;
}

inline const Operator *OperatorRegistry::get_canonical_operator(
    const Operator *op) const {
    const Operator *canonical_op = canonical_operators[get_op_index(op)];
    assert(canonical_op);
    return canonical_op;
}


#endif
