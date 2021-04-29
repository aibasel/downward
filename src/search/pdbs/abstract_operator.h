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
      Returns the effect of the abstract operator in form of a value
      change (+ or -) to an abstract state index
    */
    int get_hash_effect() const {
        return hash_effect;
    }

    /*
      Returns the cost of the abstract operator (same as the cost of
      the original concrete operator)
    */
    int get_cost() const {
        return cost;
    }

    void dump(const Projection &projection) const;
};

/*
  For each concrete operator, compute the set of all induced abstract
  operators.
 */
extern std::vector<AbstractOperator> compute_abstract_operators(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const std::vector<int> &operator_costs);
}

#endif
