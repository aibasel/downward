#ifndef EVALUATORS_MODIFY_COSTS_EVALUATOR_H
#define EVALUATORS_MODIFY_COSTS_EVALUATOR_H

#include "../evaluator.h"
#include "../operator_cost.h"

#include <memory.h>

namespace cost_adapted_evaluator {
class TaskIndependentModifyCostsEvaluator : public TaskIndependentEvaluator {
    std::shared_ptr<TaskIndependentEvaluator> nested;
    OperatorCost cost_type;
    const std::string description;
    utils::Verbosity verbosity;

    virtual std::shared_ptr<Evaluator> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task) const override;
public:
    TaskIndependentModifyCostsEvaluator(
        std::shared_ptr<TaskIndependentEvaluator> nested,
        OperatorCost cost_type, const std::string &description,
        utils::Verbosity verbosity);
};
}

#endif
