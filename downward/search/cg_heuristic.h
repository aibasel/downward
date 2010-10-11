#ifndef CG_HEURISTIC_H
#define CG_HEURISTIC_H

#include "heuristic.h"

#include <string>

class CGCache;
class DomainTransitionGraph;
class State;

class CGHeuristic : public Heuristic {
    CGCache *cache;
    int cache_hits;
    int cache_misses;

    enum {QUITE_A_LOT = 1000000};
    int helpful_transition_extraction_counter;

    void setup_domain_transition_graphs();
    int get_transition_cost(const State &state, DomainTransitionGraph *dtg, int start_val, int goal_val);
    void mark_helpful_transitions(const State &state, DomainTransitionGraph *dtg, int to);
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    CGHeuristic();
    ~CGHeuristic();
    virtual bool dead_ends_are_reliable() {return false; }

    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
