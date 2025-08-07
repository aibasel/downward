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
    WrapArgs <
         const std::vector<
              std::shared_ptr<
                  Evaluator
              >
         >,
         const std::string,
         utils::Verbosity
    >;
class SumEvaluator : public combining_evaluator::CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;
public:
    SumEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::vector<std::shared_ptr<Evaluator>> &evals,
            const std::string &description, utils::Verbosity verbosity);
};
}

#endif
