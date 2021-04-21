#ifndef PDBS_ABSTRACT_OPERATOR_H
#define PDBS_ABSTRACT_OPERATOR_H

#include "types.h"

#include "../operator_id.h"
#include "../task_proxy.h"

#include <vector>

namespace pdbs {
class PerfectHashFunction;
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

    int cost;

    /*
      Preconditions for the progression: correspond to normal
      preconditions and prevail of concrete operators.
      Preconditions for the regression: correspond to normal
      effects and prevail of concrete operators.
    */
    std::vector<FactPair> preconditions;

    /*
      Effect of the operator in form of a value change (+ or -) to an
      abstract state index.
    */
    int hash_effect;
public:
    AbstractOperator(
        int cost,
        std::vector<FactPair> &&preconditions,
        int hash_effect);
    ~AbstractOperator();

    /*
      Returns variable value pairs which represent the preconditions of
      the abstract operator in a progression/regression search.
    */
    const std::vector<FactPair> &get_preconditions() const {
        return preconditions;
    }

    /*
      Apply this operator to the given state by adding its hash effect to
      the state index.
    */
    std::size_t apply_to_state(std::size_t state_index) const;

    /*
      Returns the cost of the abstract operator (same as the cost of
      the original concrete operator)
    */
    int get_cost() const {return cost;}
    void dump(const Pattern &pattern,
              const VariablesProxy &variables) const;
};

enum class OperatorType {
    Regression,
    Progression,
    RegressionAndProgression,
};

class AbstractOperators {
    const Projection &projection;
    const PerfectHashFunction &hash_function;
    const OperatorType operator_type;

    std::vector<AbstractOperator> regression_operators;
    std::vector<AbstractOperator> progression_operators;
    std::vector<OperatorID> abstract_to_concrete_op_ids;

    /*
      Recursive method; called by build_abstract_operators. In the case
      of a precondition with value = -1 in the concrete operator, all
      multiplied out abstract operators are computed, i.e. for all
      possible values of the variable (with precondition = -1), one
      abstract operator with a concrete value (!= -1) is computed.
    */
    void multiply_out(
        int pos,
        int cost,
        std::vector<FactPair> &prev_pairs,
        std::vector<FactPair> &pre_pairs,
        std::vector<FactPair> &eff_pairs,
        const std::vector<FactPair> &effects_without_pre);

    /*
      Computes all abstract operators for a given concrete operator.
      Initializes data structures for initial call to recursive method
      multiply_out.
    */
    void build_abstract_operators(const OperatorProxy &op, int cost);

    /*
      For each concrete operator, compute the set of all induced abstract
      operators by calling build_abstract_operators. If
      compute_op_id_mapping is true, also compute a mapping from abstract
      op ids to concrete op ids.
     */
    void compute_abstract_operators(
        const std::vector<int> &operator_costs, bool compute_op_id_mapping);
public:
    AbstractOperators(
        const Projection &projection,
        const PerfectHashFunction &hash_function,
        OperatorType operator_type,
        const std::vector<int> &operator_costs,
        bool compute_op_id_mapping);

    const std::vector<AbstractOperator> &get_regression_operators() const {
        return regression_operators;
    }

    const std::vector<AbstractOperator> &get_progression_operators() const {
        return progression_operators;
    }

    const std::vector<OperatorID> &get_abstract_to_concrete_op_ids() const {
        return abstract_to_concrete_op_ids;
    }
};
}

#endif
