#ifndef HEURISTICS_GOAL_COUNT_HEURISTIC_H
#define HEURISTICS_GOAL_COUNT_HEURISTIC_H

#include "../heuristic.h"

namespace goal_count_heuristic {
class GoalCountHeuristic : public Heuristic {
    // caching the goals, avoiding FactProxy
    std::vector<std::pair<int,int>> goals;
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    GoalCountHeuristic(
        const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
        const std::string &description, utils::Verbosity verbosity);
};
}

#endif
