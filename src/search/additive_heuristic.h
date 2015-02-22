#ifndef ADDITIVE_HEURISTIC_H
#define ADDITIVE_HEURISTIC_H

#include "priority_queue.h"
#include "relaxation_heuristic.h"

#include <cassert>

class State;


class AdditiveHeuristic : public RelaxationHeuristic {
    /* Costs larger than MAX_COST_VALUE are clamped to max_value. The
       precise value (100M) is a bit of a hack, since other parts of
       the code don't reliably check against overflow as of this
       writing. With a value of 100M, we want to ensure that even
       weighted A* with a weight of 10 will have f values comfortably
       below the signed 32-bit int upper bound.
     */
    static const int MAX_COST_VALUE = 100000000;

    AdaptiveQueue<Proposition *> queue;
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
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

    // Common part of h^add and h^ff computation.
    int compute_add_and_ff(const State &state);
public:
    AdditiveHeuristic(const Options &options);
    ~AdditiveHeuristic();
};

#endif
