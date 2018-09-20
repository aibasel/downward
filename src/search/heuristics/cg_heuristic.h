#ifndef HEURISTICS_CG_HEURISTIC_H
#define HEURISTICS_CG_HEURISTIC_H

#include "../heuristic.h"

#include "../algorithms/priority_queues.h"

#include <string>
#include <vector>

namespace domain_transition_graph {
class DomainTransitionGraph;
struct ValueNode;
}

class GlobalState;
class State;

namespace cg_heuristic {
class CGCache;

class CGHeuristic : public Heuristic {
    std::vector<priority_queues::AdaptiveQueue<domain_transition_graph::ValueNode *> *> prio_queues;
    std::vector<domain_transition_graph::DomainTransitionGraph *> transition_graphs;

    CGCache *cache;
    int cache_hits;
    int cache_misses;

    int helpful_transition_extraction_counter;

    int min_action_cost;

    void setup_domain_transition_graphs();
    int get_transition_cost(
        const State &state,
        domain_transition_graph::DomainTransitionGraph *dtg,
        int start_val,
        int goal_val);
    void mark_helpful_transitions(
        const State &state,
        domain_transition_graph::DomainTransitionGraph *dtg,
        int to);
protected:
    virtual int compute_heuristic(const GlobalState &state);
public:
    CGHeuristic(const options::Options &opts);
    ~CGHeuristic();
    virtual bool dead_ends_are_reliable() const;
};
}

#endif
