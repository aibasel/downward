
#ifndef SEARCH_ALGORITHMS_PLAN_IMPROVEMENT_SEARCH_H
#define SEARCH_ALGORITHMS_PLAN_IMPROVEMENT_SEARCH_H

#include "../search_algorithm.h"

#include <memory>

namespace plan_improvement_search {

class PlanImprovementSearch : public SearchAlgorithm {
    std::shared_ptr<TaskIndependentSearchAlgorithm> search_config;

    bool inner_complete_within_bound = false;

    SearchStatus step() override;

public:
    PlanImprovementSearch(
        const std::shared_ptr<AbstractTask> &task,
        const std::shared_ptr<TaskIndependentSearchAlgorithm> &search_config,
        OperatorCost cost_type, int bound, double max_time,
        const std::string &description, utils::Verbosity verbosity);

    void print_statistics() const override;
    bool is_complete_within_bound() const override;
};

}

#endif
