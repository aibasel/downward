#ifndef ADDITIVE_HEURISTIC_H
#define ADDITIVE_HEURISTIC_H

#include "relaxation_heuristic.h"
#include <cassert>
#include <ext/hash_set>

/*
  TODO:
  Some basic data structures, like UnaryOperator, Proposition, etc.,
  are taken from "ff_heuristic.h". This needs to be refactored,
  along with the enormous duplication between the two heuristics.
*/

class AdditiveHeuristic : public RelaxationHeuristic {
    typedef __gnu_cxx::hash_set<const Operator *, hash_operator_ptr> RelaxedPlan;

    typedef std::vector<Proposition *> Bucket;
    std::vector<Bucket> reachable_queue;

    void setup_exploration_queue();
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration();
    void collect_relaxed_plan(Proposition *goal, RelaxedPlan &relaxed_plan);

    void enqueue_if_necessary(Proposition *prop, int cost, UnaryOperator *op) {
        assert(cost >= 0);
        if(prop->h_add_cost == -1 || prop->h_add_cost > cost) {
            prop->h_add_cost = cost;
            prop->reached_by = op;
            if(cost >= reachable_queue.size())
                reachable_queue.resize(cost + 1);
            reachable_queue[cost].push_back(prop);
        }
        assert(prop->h_add_cost != -1 &&
               prop->h_add_cost <= cost);
    }
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    AdditiveHeuristic(bool use_cache=false);
    ~AdditiveHeuristic();
};

#endif
