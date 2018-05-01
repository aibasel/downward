#ifndef HEURISTICS_ADDITIVE_HEURISTIC_H
#define HEURISTICS_ADDITIVE_HEURISTIC_H

#include "relaxation_heuristic.h"

#include "../algorithms/priority_queues.h"
#include "../utils/collections.h"

#include <cassert>

class State;

namespace additive_heuristic {
using relaxation_heuristic::Proposition;
using relaxation_heuristic::UnaryOperator;

class AdditiveHeuristic : public relaxation_heuristic::RelaxationHeuristic {
    /* Costs larger than MAX_COST_VALUE are clamped to max_value. The
       precise value (100M) is a bit of a hack, since other parts of
       the code don't reliably check against overflow as of this
       writing. With a value of 100M, we want to ensure that even
       weighted A* with a weight of 10 will have f values comfortably
       below the signed 32-bit int upper bound.
     */
    static const int MAX_COST_VALUE = 100000000;

    priority_queues::AdaptiveQueue<Proposition *> queue;
    bool did_write_overflow_warning;

    void setup_exploration_queue();
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration();
    void mark_preferred_operators(const State &state, Proposition *goal);

    void enqueue_if_necessary(Proposition *prop, int cost, UnaryOperator *op) {
        assert(cost >= 0);
        if (prop->cost == -1 || prop->cost > cost) {
            prop->cost = cost;
            prop->reached_by = op;
            queue.push(cost, prop);
        }
        assert(prop->cost != -1 && prop->cost <= cost);
    }

    void increase_cost(int &cost, int amount) {
        assert(cost >= 0);
        assert(amount >= 0);
        cost += amount;
        if (cost > MAX_COST_VALUE) {
            write_overflow_warning();
            cost = MAX_COST_VALUE;
        }
    }

    void write_overflow_warning();

    int compute_heuristic(const State &state);
protected:
    virtual int compute_heuristic(const GlobalState &global_state);

    // Common part of h^add and h^ff computation.
    int compute_add_and_ff(const State &state);
public:
    explicit AdditiveHeuristic(const options::Options &opts);
    ~AdditiveHeuristic();

    /*
      TODO: The two methods below are temporarily needed for the CEGAR
      heuristic. In the long run it might be better to split the
      computation from the heuristic class. Then the CEGAR code could
      use the computation object instead of the heuristic.
    */
    void compute_heuristic_for_cegar(const State &state);

    int get_cost_for_cegar(int var, int value) const {
        assert(utils::in_bounds(var, propositions));
        assert(utils::in_bounds(value, propositions[var]));
        return propositions[var][value].cost;
    }
};
}

#endif
