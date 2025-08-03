#ifndef EVALUATORS_SUM_EVALUATOR_H
#define EVALUATORS_SUM_EVALUATOR_H

#include "combining_evaluator.h"

#include <memory>
#include <vector>

namespace plugins {
class Options;
}

template<typename T>
class TaskIndependentComponentType;

namespace sum_evaluator {
using SumEvaluatorArgs = 
    std::tuple<
         const std::vector<
              std::shared_ptr<
                   TaskIndependentComponentType<
                        Evaluator
                   >
              >
         >,
    const std::string,
    utils::Verbosity>;
class SumEvaluator : public combining_evaluator::CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;
public:
    SumEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::vector<std::shared_ptr<Evaluator>> &evals,
            const std::string &description, utils::Verbosity verbosity);
};

//class TaskIndependentSumEvaluator : public TaskIndependentComponent<Evaluator> {
//    std::vector<std::shared_ptr<TaskIndependentComponent<Evaluator>>> subevaluators;
//    virtual std::shared_ptr<Evaluator> create_task_specific(
//        const std::shared_ptr<AbstractTask> &task,
//        std::unique_ptr<ComponentMap> &component_map,
//        int depth) const override;
//public:
//    TaskIndependentSumEvaluator(
//        const std::vector<std::shared_ptr<TaskIndependentComponent<Evaluator>>> &subevaluators,
//        const std::string &description, utils::Verbosity verbosity);
//
//    virtual ~TaskIndependentSumEvaluator() override = default;
//};
}

#endif
