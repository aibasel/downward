#ifndef HEURISTICS_GOAL_COUNT_HEURISTIC_H
#define HEURISTICS_GOAL_COUNT_HEURISTIC_H

#include "../heuristic.h"

namespace goal_count_heuristic {
class GoalCountHeuristic : public Heuristic {
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit GoalCountHeuristic(const options::Options &opts);
};
}

#endif
