#ifndef MAX_EVALUATOR_H
#define MAX_EVALUATOR_H

#include "combining_evaluator.h"

#include <vector>

class MaxEvaluator : public CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;
public:
    explicit MaxEvaluator(const std::vector<ScalarEvaluator *> &subevaluators);
    virtual ~MaxEvaluator() override;
};

#endif
