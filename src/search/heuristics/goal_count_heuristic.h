#ifndef HEURISTICS_GOAL_COUNT_HEURISTIC_H
#define HEURISTICS_GOAL_COUNT_HEURISTIC_H

#include "../heuristic.h"

namespace goal_count_heuristic {
class GoalCountHeuristic : public Heuristic {
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);
public:
    GoalCountHeuristic(const options::Options &options);
    ~GoalCountHeuristic();
};
}

#endif
