#ifndef CG_HEURISTIC_H
#define CG_HEURISTIC_H

#include "heuristic.h"
#include "priority_queue.h"

#include <string>
#include <vector>

class CGCache;
class DomainTransitionGraph;
class GlobalState;
class State;
struct ValueNode;

class CGHeuristic : public Heuristic {
    std::vector<AdaptiveQueue<ValueNode *> *> prio_queues;
    std::vector<DomainTransitionGraph *> transition_graphs;

    CGCache *cache;
    int cache_hits;
    int cache_misses;

    int helpful_transition_extraction_counter;

    int min_action_cost;

    void setup_domain_transition_graphs();
    int get_transition_cost(const State &state, DomainTransitionGraph *dtg, int start_val, int goal_val);
    void mark_helpful_transitions(const State &state, DomainTransitionGraph *dtg, int to);
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);
public:
    CGHeuristic(const Options &opts);
    ~CGHeuristic();
    virtual bool dead_ends_are_reliable() const;
};

#endif
