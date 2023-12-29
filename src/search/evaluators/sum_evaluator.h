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
    explicit SumEvaluator(const plugins::Options &opts);
    explicit SumEvaluator(
            std::vector<std::shared_ptr<Evaluator>> subevaluators,
            const std::string &name,
            utils::Verbosity verbosity);
    virtual ~SumEvaluator() override;
};


class TaskIndependentSumEvaluator : public combining_evaluator::TaskIndependentCombiningEvaluator {
public:
    explicit TaskIndependentSumEvaluator(
            std::vector<std::shared_ptr<TaskIndependentEvaluator>> subevaluators,
            const std::string &name,
                                         utils::Verbosity verbosity);

    virtual ~TaskIndependentSumEvaluator()  override = default;

    std::shared_ptr<Evaluator>
    create_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                         int depth = -1) const override;
};
}

#endif
