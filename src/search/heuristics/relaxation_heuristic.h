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

struct UnaryOperator {
    int operator_no; // -1 for axioms; index into the task's operators otherwise
    std::vector<Proposition *> precondition;
    Proposition *effect;
    int base_cost;

    int unsatisfied_preconditions;
    int cost; // Used for h^max cost or h^add cost;
              // includes operator cost (base_cost)
    UnaryOperator(const std::vector<Proposition *> &pre, Proposition *eff,
                  int operator_no_, int base)
        : operator_no(operator_no_), precondition(pre), effect(eff),
          base_cost(base) {}
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
    void build_unary_operators(const OperatorProxy &op, int op_no);
    void simplify();
protected:
    std::vector<UnaryOperator> unary_operators;
    std::vector<std::vector<Proposition>> propositions;
    std::vector<Proposition *> goal_propositions;

    Proposition *get_proposition(const FactProxy &fact);
    virtual int compute_heuristic(const GlobalState &state) = 0;
public:
    RelaxationHeuristic(const options::Options &opts);
    virtual ~RelaxationHeuristic();
    virtual bool dead_ends_are_reliable() const;
};
}

#endif
