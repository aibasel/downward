#ifndef EVALUATORS_COST_ADAPTED_EVALUATOR_H
#define EVALUATORS_COST_ADAPTED_EVALUATOR_H

#include "../component.h"
#include "../evaluator.h"
#include "../operator_cost.h"

#include <memory.h>

namespace cost_adapted_evaluator {
class TaskIndependentCostAdaptedEvaluator : public TaskIndependentEvaluator {
    std::shared_ptr<TaskIndependentEvaluator> nested;
    OperatorCost cost_type;

    virtual std::shared_ptr<Evaluator> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task, components::Cache &cache) const override;
public:
    TaskIndependentCostAdaptedEvaluator(
        std::shared_ptr<TaskIndependentEvaluator> nested,
        OperatorCost cost_type);
};
}

#endif
