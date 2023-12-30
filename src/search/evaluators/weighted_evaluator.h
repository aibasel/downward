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
    explicit WeightedEvaluator(
            std::shared_ptr<Evaluator> evaluator,
            int weight,
            const std::string &name,
            utils::Verbosity verbosity);
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
protected:
    std::string get_product_name() const override { return "WeightedEvaluator";}
public:
    explicit TaskIndependentWeightedEvaluator(
                                              std::shared_ptr<TaskIndependentEvaluator> evaluator,
                                              int weight,
                                              const std::string &name,
                                              utils::Verbosity verbosity);

    virtual ~TaskIndependentWeightedEvaluator() override = default;

    using AbstractProduct = Evaluator;
    using ConcreteProduct = WeightedEvaluator;


    std::shared_ptr<AbstractProduct>
    get_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                      int depth = -1) const override;

    std::shared_ptr<ConcreteProduct> create_ts(
            const std::shared_ptr<AbstractTask> &task,
            std::unique_ptr<ComponentMap> &component_map,
            int depth) const;
};
}

#endif
