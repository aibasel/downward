#ifndef HEURISTICS_COST_ADAPTED_HEURISTIC_H
#define HEURISTICS_COST_ADAPTED_HEURISTIC_H

#include "../component.h"
#include "../evaluator.h"
#include "../operator_cost.h"

#include <memory.h>

namespace cost_adapted_heuristic {
class TaskIndependentCostAdaptedHeuristic : public TaskIndependentEvaluator {
    std::shared_ptr<TaskIndependentEvaluator> nested;
    OperatorCost cost_type;

    virtual std::shared_ptr<Evaluator> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task, components::Cache &cache) const override;
public:
    TaskIndependentCostAdaptedHeuristic(
        std::shared_ptr<TaskIndependentEvaluator> nested,
        OperatorCost cost_type);
};
}

#endif
