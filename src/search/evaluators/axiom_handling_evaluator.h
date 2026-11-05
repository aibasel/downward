#ifndef EVALUATORS_AXIOM_HANDLING_EVALUATOR_H
#define EVALUATORS_AXIOM_HANDLING_EVALUATOR_H

#include "../evaluator.h"

#include "../tasks/default_value_axioms_task.h"

#include <memory.h>

namespace axiom_handling_evaluator {
class TaskIndependentAxiomHandlingEvaluator : public TaskIndependentEvaluator {
    std::shared_ptr<TaskIndependentEvaluator> nested;
    tasks::AxiomHandlingType axioms;
    const std::string description;
    utils::Verbosity verbosity;

    virtual std::shared_ptr<Evaluator> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task) const override;
public:
    TaskIndependentAxiomHandlingEvaluator(
        std::shared_ptr<TaskIndependentEvaluator> nested,
        tasks::AxiomHandlingType axioms, const std::string &description,
        utils::Verbosity verbosity);
};

std::shared_ptr<TaskIndependentEvaluator> wrap_in_axiom_handling_evaluator(
    const std::shared_ptr<TaskIndependentEvaluator> &eval,
    const plugins::Options &opts);
}

#endif
