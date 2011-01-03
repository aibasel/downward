#ifndef RELAXATION_HEURISTIC_H
#define RELAXATION_HEURISTIC_H

#include "heuristic.h"

#include <vector>

class Operator;
class State;

class Proposition;
class UnaryOperator;

struct UnaryOperator {
    const Operator *op;
    std::vector<Proposition *> precondition;
    Proposition *effect;
    int base_cost;

    int unsatisfied_preconditions;
    int cost; // Used for h^max cost or h^add cost;
              // includes operator cost (base_cost)
    UnaryOperator(const std::vector<Proposition *> &pre, Proposition *eff,
                  const Operator *the_op, int base)
        : op(the_op), precondition(pre), effect(eff), base_cost(base) {}
};

struct Proposition {
    bool is_goal;
    int id;
    std::vector<UnaryOperator *> precondition_of;

    int cost; // Used for h^max cost or h^add cost
    UnaryOperator *reached_by;
    bool marked; // used when computing preferred operators for h^add and h^FF

    Proposition(int id_) {
        id = id_;
        is_goal = false;
        cost = -1;
        reached_by = 0;
        marked = false;
    }
};

class RelaxationHeuristic : public Heuristic {
    void build_unary_operators(const Operator &op);
    void simplify();
protected:
    std::vector<UnaryOperator> unary_operators;
    std::vector<std::vector<Proposition> > propositions;
    std::vector<Proposition *> goal_propositions;

    virtual void initialize();
    virtual int compute_heuristic(const State &state) = 0;
public:
    RelaxationHeuristic(const HeuristicOptions &options);
    virtual ~RelaxationHeuristic();
};

#endif
