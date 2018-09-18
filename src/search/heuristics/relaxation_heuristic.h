#ifndef HEURISTICS_RELAXATION_HEURISTIC_H
#define HEURISTICS_RELAXATION_HEURISTIC_H

#include "../heuristic.h"

#include <vector>

class FactProxy;
class GlobalState;
class OperatorProxy;

namespace relaxation_heuristic {
struct Proposition;
struct UnaryOperator;

struct Proposition {
    Proposition();

    bool is_goal;
    std::vector<UnaryOperator *> precondition_of;

    int cost; // Used for h^max cost or h^add cost
    UnaryOperator *reached_by;
    bool marked; // used when computing preferred operators for h^add and h^FF
};

struct UnaryOperator {
    UnaryOperator(const std::vector<Proposition *> &pre, Proposition *eff,
                  int operator_no, int base);

    int operator_no; // -1 for axioms; index into the task's operators otherwise
    std::vector<Proposition *> precondition;
    Proposition *effect;
    int base_cost;

    int unsatisfied_preconditions;
    int cost; // Used for h^max cost or h^add cost;
              // includes operator cost (base_cost)
};

class RelaxationHeuristic : public Heuristic {
    void build_unary_operators(const OperatorProxy &op, int operator_no);
    int get_proposition_id(const Proposition &prop) const;
    int get_operator_id(const UnaryOperator &op) const;
    void simplify();

    /*
      proposition_offsets[var_no] is the index of the first proposition
      related to variable var_no in propositions
    */
    std::vector<int> proposition_offsets;
protected:
    std::vector<UnaryOperator> unary_operators;
    std::vector<Proposition> propositions;
    std::vector<Proposition *> goal_propositions;

    /*
      TODO: The const method is only needed for the CEGAR hack
      in the additive heuristic and should eventually go away.
    */
    const Proposition *get_proposition(int var, int value) const;
    Proposition *get_proposition(int var, int value);
    Proposition *get_proposition(const FactProxy &fact);
    virtual int compute_heuristic(const GlobalState &state) = 0;
public:
    explicit RelaxationHeuristic(const options::Options &options);
    virtual ~RelaxationHeuristic();
    virtual bool dead_ends_are_reliable() const;
};
}

#endif
