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
    int base_cost; // 0 for axioms, 1 for regular operators

    int unsatisfied_preconditions;
    int h_add_cost;
    int h_max_cost;
    UnaryOperator(const std::vector<Proposition *> &pre, Proposition *eff,
                  const Operator *the_op, int base)
        : op(the_op), precondition(pre), effect(eff), base_cost(base) {}
};

struct Proposition {
    bool is_goal;
    int id;
    std::vector<UnaryOperator *> precondition_of;

    int h_add_cost;
    int h_max_cost;
    UnaryOperator *reached_by;
    bool marked; // used when computing preferred operators for h^add and h^FF

    Proposition(int id_) {
        id = id_;
        is_goal = false;
        h_add_cost = -1;
        reached_by = 0;
        marked = false;
    }
};

struct hash_operator_ptr {
    size_t operator()(const Operator *key) const {
        return reinterpret_cast<unsigned long>(key);
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
    RelaxationHeuristic();
    virtual ~RelaxationHeuristic();
};

#endif
