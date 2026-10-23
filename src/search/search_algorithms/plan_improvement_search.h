#ifndef SEARCH_ALGORITHMS_PLAN_IMPROVEMENT_SEARCH_H
#define SEARCH_ALGORITHMS_PLAN_IMPROVEMENT_SEARCH_H

#include "../search_algorithm.h"

#include "../plan_improvement/plan_improvement.h"
#include "../utils/timer.h"

#include <memory>

namespace plan_improvement_search {
class PlanImprovementSearch : public SearchAlgorithm {
    std::shared_ptr<TaskIndependentSearchAlgorithm> search_config;

    bool inner_complete_within_bound = false;
    std::shared_ptr<PlanImprovement> improvement;
    virtual SearchStatus step() override;
    int initial_plan_length = 0;
    int final_plan_length = 0;
    int initial_plan_cost = 0;
    int final_plan_cost = 0;

    utils::Timer improvement_timer{false};

public:
    PlanImprovementSearch(
        const std::shared_ptr<AbstractTask> &task,
        const std::shared_ptr<TaskIndependentSearchAlgorithm> &search_config,
        OperatorCost cost_type, int bound, double max_time,
        const std::shared_ptr<PlanImprovement> &improvement,
        const std::string &description, utils::Verbosity verbosity);

    void print_statistics() const override;
    bool is_complete_within_bound() const override;
};

}

#endif
