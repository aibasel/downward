#ifndef MAX_HEURISTIC_H
#define MAX_HEURISTIC_H

#include "priority_queue.h"
#include "relaxation_heuristic.h"
#include <cassert>

class HSPMaxHeuristic : public RelaxationHeuristic {
    AdaptiveQueue<Proposition *> queue;

    void setup_exploration_queue();
    void setup_exploration_queue_state(const GlobalState &state);
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
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);
public:
    HSPMaxHeuristic(const Options &options);
    ~HSPMaxHeuristic();
};

#endif
