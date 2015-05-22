#ifndef BLIND_SEARCH_HEURISTIC_H
#define BLIND_SEARCH_HEURISTIC_H

#include "heuristic.h"

class BlindSearchHeuristic : public Heuristic {
    int min_operator_cost;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);
public:
    BlindSearchHeuristic(const Options &options);
    ~BlindSearchHeuristic();
};

#endif
