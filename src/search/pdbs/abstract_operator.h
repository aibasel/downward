#ifndef PDBS_ABSTRACT_OPERATOR_H
#define PDBS_ABSTRACT_OPERATOR_H

#include "types.h"

#include "../task_proxy.h"

#include <vector>

namespace utils {
class LogProxy;
}

namespace pdbs {
class Projection;

class AbstractOperator {
    /*
      This class represents an abstract operator how it is needed for
      the regression search performed during the PDB-construction. As
      all abstract states are represented as a number, abstract
      operators don't have "usual" effects but "hash effects", i.e. the
      change (as number) the abstract operator implies on a given
      abstract state.
    */

    int concrete_op_id;
    int cost;

    /*
      Preconditions for the regression search, corresponds to normal
      effects and prevail of concrete operators.
    */
    std::vector<FactPair> regression_preconditions;

    /*
      Effect of the operator during regression search on a given
      abstract state number.
    */
    int hash_effect;
public:
    AbstractOperator(
        int concrete_op_id,
        int cost,
        std::vector<FactPair> &&regression_preconditions,
        int hash_effect);

    /*
      Returns variable value pairs which represent the preconditions of
      the abstract operator in a regression search
    */
    const std::vector<FactPair> &get_regression_preconditions() const {
        return regression_preconditions;
    }

    /*
      Returns the effect of the abstract operator in form of a value
      change (+ or -) to an abstract state index
    */
    int get_hash_effect() const {
        return hash_effect;
    }

    int get_concrete_op_id() const {
        return concrete_op_id;
    }

    /*
      Returns the cost of the abstract operator (same as the cost of
      the original concrete operator)
    */
    int get_cost() const {
        return cost;
    }

    void dump(const Pattern &pattern,
              const VariablesProxy &variables,
              utils::LogProxy &log) const;
};
}

#endif
