#ifndef HEURISTICS_GOAL_COUNT_HEURISTIC_H
#define HEURISTICS_GOAL_COUNT_HEURISTIC_H

#include "../heuristic.h"


namespace GoalCountHeuristic {
class GoalCountHeuristic : public Heuristic {
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);
public:
    GoalCountHeuristic(const Options &options);
    ~GoalCountHeuristic();
};
}

#endif
