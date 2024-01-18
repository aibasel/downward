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
    explicit SumEvaluator(utils::LogProxy log,
                          std::vector<std::shared_ptr<Evaluator>> subevaluators,
                          std::basic_string<char> unparsed_config = std::string(),
                          bool use_for_reporting_minima = false,
                          bool use_for_boosting = false,
                          bool use_for_counting_evaluations = false);
    virtual ~SumEvaluator() override;
};


class TaskIndependentSumEvaluator : public combining_evaluator::TaskIndependentCombiningEvaluator {
private:
    std::string unparsed_config;
    utils::LogProxy log;
public:
    explicit TaskIndependentSumEvaluator(utils::LogProxy log,
                                         std::vector<std::shared_ptr<TaskIndependentEvaluator>> subevaluators,
                                         std::string unparsed_config = std::string(),
                                         bool use_for_reporting_minima = false,
                                         bool use_for_boosting = false,
                                         bool use_for_counting_evaluations = false);

    virtual ~TaskIndependentSumEvaluator()  override;

    virtual std::shared_ptr<combining_evaluator::CombiningEvaluator> create_task_specific_CombiningEvaluator(
            const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<SumEvaluator> create_task_specific_SumEvaluator(const std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<SumEvaluator> create_task_specific_SumEvaluator(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map, int depth = -1);
};
}

#endif
