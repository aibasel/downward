#ifndef BLIND_SEARCH_HEURISTIC_H
#define BLIND_SEARCH_HEURISTIC_H

#include "heuristic.h"

class Task;

class BlindSearchHeuristic : public Heuristic {
    int min_operator_cost;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    BlindSearchHeuristic(const Task &task_, const Options &options);
    ~BlindSearchHeuristic();
};

#endif
