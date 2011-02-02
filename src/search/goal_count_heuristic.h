#ifndef GOAL_COUNT_HEURISTIC_H
#define GOAL_COUNT_HEURISTIC_H

#include "heuristic.h"

class GoalCountHeuristic : public Heuristic {
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    GoalCountHeuristic(const HeuristicOptions &options);
    ~GoalCountHeuristic();
};

#endif
