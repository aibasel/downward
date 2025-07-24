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
    SumEvaluator(
        const std::vector<std::shared_ptr<Evaluator>> &evals,
        const std::string &description, utils::Verbosity verbosity);
};

class TaskIndependentSumEvaluator : public combining_evaluator::TaskIndependentCombiningEvaluator {
public:
    TaskIndependentSumEvaluator(
        const std::vector<std::shared_ptr<TaskIndependentEvaluator>> &subevaluators,
        const std::string &description, utils::Verbosity verbosity);

    virtual ~TaskIndependentSumEvaluator() override = default;

    virtual std::shared_ptr<Evaluator> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override;
};
}

#endif
