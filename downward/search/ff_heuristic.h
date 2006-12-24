#ifndef FF_HEURISTIC_H
#define FF_HEURISTIC_H

#include "heuristic.h"

#include <vector>
#include <ext/hash_set>

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
    UnaryOperator(const std::vector<Proposition *> &pre, Proposition *eff,
		  const Operator *the_op, int base)
	: op(the_op), precondition(pre), effect(eff), base_cost(base) {}
};

struct Proposition {
    bool is_goal;
    std::vector<UnaryOperator *> precondition_of;

    int h_add_cost;
    UnaryOperator *reached_by;

    Proposition() {
	is_goal = false;
	h_add_cost = -1;
	reached_by = 0;
    }
};

struct hash_operator_ptr {
    size_t operator()(const Operator *key) const {
	return reinterpret_cast<unsigned long>(key);
    }
};

class FFHeuristic : public Heuristic {
    typedef __gnu_cxx::hash_set<const Operator *, hash_operator_ptr> RelaxedPlan;

    std::vector<UnaryOperator> unary_operators;
    std::vector<std::vector<Proposition> > propositions;
    std::vector<Proposition *> goal_propositions;

    Proposition **reachable_queue_start;
    Proposition **reachable_queue_read_pos;
    Proposition **reachable_queue_write_pos;

    void build_unary_operators(const Operator &op);
    void simplify();

    void setup_exploration_queue();
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration();
    void collect_relaxed_plan(Proposition *goal, RelaxedPlan &relaxed_plan);

    int compute_hsp_add_heuristic();
    int compute_ff_heuristic();
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    FFHeuristic(bool use_cache=false);
    ~FFHeuristic();
};

#endif
