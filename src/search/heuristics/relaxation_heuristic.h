#ifndef HEURISTICS_RELAXATION_HEURISTIC_H
#define HEURISTICS_RELAXATION_HEURISTIC_H

#include "../heuristic.h"

#include "../utils/collections.h"

#include <cassert>
#include <vector>

class FactProxy;
class GlobalState;
class OperatorProxy;

namespace relaxation_heuristic {
struct Proposition;
struct UnaryOperator;

using PropID = int;
using OpID = int;

const OpID NO_OP = -1;

struct Proposition {
    Proposition();

    bool is_goal;
    std::vector<OpID> precondition_of;

    int cost; // Used for h^max cost or h^add cost
    PropID reached_by;
    bool marked; // used when computing preferred operators for h^add and h^FF
};

struct UnaryOperator {
    UnaryOperator(const std::vector<PropID> &pre, PropID eff,
                  int operator_no, int base);

    int operator_no; // -1 for axioms; index into the task's operators otherwise
    std::vector<PropID> precondition;
    PropID effect;
    int base_cost;

    int unsatisfied_preconditions;
    int cost; // Used for h^max cost or h^add cost;
              // includes operator cost (base_cost)
};

class RelaxationHeuristic : public Heuristic {
    void build_unary_operators(const OperatorProxy &op, int operator_no);
    void simplify();

    // proposition_offsets[var_no]: first PropID related to variable var_no
    std::vector<PropID> proposition_offsets;
protected:
    std::vector<UnaryOperator> unary_operators;
    std::vector<Proposition> propositions;
    std::vector<PropID> goal_propositions;

    /*
      TODO: Some of these protected methods are only needed for the
      CEGAR hack in the additive heuristic and should eventually go
      away.
    */
    PropID get_prop_id(const Proposition &prop) const {
        PropID prop_id = &prop - propositions.data();
        assert(utils::in_bounds(prop_id, propositions));
        return prop_id;
    }

    OpID get_op_id(const UnaryOperator &op) const {
        OpID op_id = &op - unary_operators.data();
        assert(utils::in_bounds(op_id, unary_operators));
        return op_id;
    }

    PropID get_prop_id(int var, int value) const;
    PropID get_prop_id(const FactProxy &fact) const;

    Proposition *get_proposition(PropID prop_id) {
        return &propositions[prop_id];
    }
    UnaryOperator *get_operator(OpID op_id) {
        return &unary_operators[op_id];
    }

    const Proposition *get_proposition(int var, int value) const;
    Proposition *get_proposition(int var, int value);
    Proposition *get_proposition(const FactProxy &fact);
    virtual int compute_heuristic(const GlobalState &state) = 0;
public:
    explicit RelaxationHeuristic(const options::Options &options);
    virtual ~RelaxationHeuristic() override;
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
