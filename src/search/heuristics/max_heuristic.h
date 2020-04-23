#ifndef HEURISTICS_MAX_HEURISTIC_H
#define HEURISTICS_MAX_HEURISTIC_H

#include "relaxation_heuristic.h"

#include "../algorithms/priority_queues.h"

#include <cassert>

namespace max_heuristic {
using relaxation_heuristic::PropID;
using relaxation_heuristic::OpID;

using relaxation_heuristic::Proposition;
using relaxation_heuristic::UnaryOperator;

class HSPMaxHeuristic : public relaxation_heuristic::RelaxationHeuristic {
    priority_queues::AdaptiveQueue<PropID> queue;

    void setup_exploration_queue();
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration();

    void enqueue_if_necessary(PropID prop_id, int cost) {
        assert(cost >= 0);
        Proposition *prop = get_proposition(prop_id);
        if (prop->cost == -1 || prop->cost > cost) {
            prop->cost = cost;
            queue.push(cost, prop_id);
        }
        assert(prop->cost != -1 && prop->cost <= cost);
    }
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit HSPMaxHeuristic(const options::Options &opts);
};
}

#endif
