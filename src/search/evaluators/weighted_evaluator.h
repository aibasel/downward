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
    explicit WeightedEvaluator(const plugins::Options &opts);
    explicit WeightedEvaluator(utils::LogProxy log,
                               std::shared_ptr<Evaluator> evaluator,
                               int weight,
                               std::basic_string<char> unparsed_config = std::string(),
                               bool use_for_reporting_minima = false,
                               bool use_for_boosting = false,
                               bool use_for_counting_evaluations = false);
    virtual ~WeightedEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};


class TaskIndependentWeightedEvaluator : public TaskIndependentEvaluator {
private:
    std::shared_ptr<TaskIndependentEvaluator> evaluator;
    int weight;
public:
    explicit TaskIndependentWeightedEvaluator(utils::LogProxy log,
                                              std::shared_ptr<TaskIndependentEvaluator> evaluator,
                                              int weight,
                                              std::string unparsed_config = std::string(),
                                              bool use_for_reporting_minima = false,
                                              bool use_for_boosting = false,
                                              bool use_for_counting_evaluations = false);

    virtual ~TaskIndependentWeightedEvaluator()  override;

    virtual std::shared_ptr<Evaluator> create_task_specific_Evaluator(const std::shared_ptr<AbstractTask> &task, int depth = -1) override;

    virtual std::shared_ptr<Evaluator> create_task_specific_Evaluator(
            const std::shared_ptr<AbstractTask> &task,
        std::shared_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<WeightedEvaluator> create_task_specific_WeightedEvaluator(const std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<WeightedEvaluator> create_task_specific_WeightedEvaluator(const std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth = -1);
};
}

#endif
