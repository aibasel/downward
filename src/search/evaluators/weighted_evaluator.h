#ifndef EVALUATORS_WEIGHTED_EVALUATOR_H
#define EVALUATORS_WEIGHTED_EVALUATOR_H

#include "../evaluator.h"

#include <memory>

namespace weighted_evaluator {
class WeightedEvaluator : public TaskSpecificEvaluator {
    std::shared_ptr<TaskSpecificEvaluator> evaluator;
    int weight;

public:
    WeightedEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::shared_ptr<TaskSpecificEvaluator> &eval, int weight,
        const std::string &description, utils::Verbosity verbosity);

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_path_dependent_evaluators(
        std::set<TaskSpecificEvaluator *> &evals) override;
};
}

#endif
