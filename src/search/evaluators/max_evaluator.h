#ifndef EVALUATORS_MAX_EVALUATOR_H
#define EVALUATORS_MAX_EVALUATOR_H

#include "combining_evaluator.h"

#include <vector>


namespace MaxEvaluator {
class MaxEvaluator : public CombiningEvaluator::CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;
public:
    explicit MaxEvaluator(const std::vector<ScalarEvaluator *> &subevaluators);
    virtual ~MaxEvaluator() override;
};
}

#endif
