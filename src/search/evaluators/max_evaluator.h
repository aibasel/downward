#ifndef EVALUATORS_MAX_EVALUATOR_H
#define EVALUATORS_MAX_EVALUATOR_H

#include "combining_evaluator.h"

#include <vector>

namespace plugins {
class Options;
}

namespace max_evaluator {
class MaxEvaluator : public combining_evaluator::CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;

public:
    MaxEvaluator(
        const std::vector<std::shared_ptr<Evaluator>> &evals,
        const std::string &description, utils::Verbosity verbosity);
};
}

#endif
