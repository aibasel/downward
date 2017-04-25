#ifndef HEURISTICS_MAX_HEURISTIC_H
#define HEURISTICS_MAX_HEURISTIC_H

#include "relaxation_heuristic.h"

#include "../algorithms/priority_queues.h"

#include <cassert>

namespace max_heuristic {
using relaxation_heuristic::Proposition;
using relaxation_heuristic::UnaryOperator;

class HSPMaxHeuristic : public relaxation_heuristic::RelaxationHeuristic {
    priority_queues::AdaptiveQueue<Proposition *> queue;

    void setup_exploration_queue();
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration();

    void enqueue_if_necessary(Proposition *prop, int cost) {
        assert(cost >= 0);
        if (prop->cost == -1 || prop->cost > cost) {
            prop->cost = cost;
            queue.push(cost, prop);
        }
        assert(prop->cost != -1 && prop->cost <= cost);
    }
protected:
    virtual int compute_heuristic(const GlobalState &global_state);
public:
    HSPMaxHeuristic(const options::Options &options);
    ~HSPMaxHeuristic();
};
}

#endif
