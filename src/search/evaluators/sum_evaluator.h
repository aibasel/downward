#ifndef EVALUATORS_SUM_EVALUATOR_H
#define EVALUATORS_SUM_EVALUATOR_H

#include "combining_evaluator.h"

#include <memory>
#include <vector>

namespace plugins {
class Options;
}

namespace sum_evaluator {
class SumEvaluator : public combining_evaluator::CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;
public:
    explicit SumEvaluator(
        std::vector<std::shared_ptr<Evaluator>> subevaluators,
        const std::string &name,
        utils::Verbosity verbosity);
    virtual ~SumEvaluator() override;
};


class TaskIndependentSumEvaluator : public combining_evaluator::TaskIndependentCombiningEvaluator {
protected:
    std::string get_product_name() const override {return "SumEvaluator";}
public:
    explicit TaskIndependentSumEvaluator(
        std::vector<std::shared_ptr<TaskIndependentEvaluator>> subevaluators,
        const std::string &name,
        utils::Verbosity verbosity);

    virtual ~TaskIndependentSumEvaluator()  override = default;

    using AbstractProduct = Evaluator;
    using ConcreteProduct = SumEvaluator;

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
