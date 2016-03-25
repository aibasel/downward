#ifndef EVALUATORS_MAX_EVALUATOR_H
#define EVALUATORS_MAX_EVALUATOR_H

#include "combining_evaluator.h"

#include <vector>

namespace options {
class Options;
}

namespace max_evaluator {
class MaxEvaluator : public combining_evaluator::CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;

public:
    explicit MaxEvaluator(const options::Options &opts);
    virtual ~MaxEvaluator() override;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
};
}

#endif
