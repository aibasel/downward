#ifndef EVALUATORS_WEIGHTED_EVALUATOR_H
#define EVALUATORS_WEIGHTED_EVALUATOR_H

#include "../evaluator.h"

#include <memory>

namespace plugins {
class Options;
}

namespace weighted_evaluator {
class WeightedEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;
    int weight;

public:
    WeightedEvaluator(
        const std::shared_ptr<Evaluator> &eval, int weight,
        const std::string &description, utils::Verbosity verbosity);

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};

class TaskIndependentWeightedEvaluator : public TaskIndependentComponent<Evaluator> {
    std::shared_ptr<TaskIndependentComponent<Evaluator>> evaluator;
    int weight;
    std::shared_ptr<Evaluator> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override;
public:
    explicit TaskIndependentWeightedEvaluator(
        const std::shared_ptr<TaskIndependentComponent<Evaluator>> &eval,
        int weight,
        const std::string &description,
        utils::Verbosity verbosity);

    virtual ~TaskIndependentWeightedEvaluator() override = default;

};


}

#endif
