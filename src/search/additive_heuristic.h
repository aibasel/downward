#ifndef ADDITIVE_HEURISTIC_H
#define ADDITIVE_HEURISTIC_H

#include "priority_queue.h"
#include "relaxation_heuristic.h"
#include <cassert>

class AdditiveHeuristic : public RelaxationHeuristic {
    AdaptiveQueue<Proposition *> queue;

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
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);

    // Common part of h^add and h^ff computation.
    int compute_add_and_ff(const State &state);
public:
    AdditiveHeuristic(const HeuristicOptions &options);
    ~AdditiveHeuristic();

    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
