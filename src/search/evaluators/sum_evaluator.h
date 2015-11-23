#ifndef SUM_EVALUATOR_H
#define SUM_EVALUATOR_H

#include "combining_evaluator.h"

#include <vector>

class Options;

class SumEvaluator : public CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;
public:
    explicit SumEvaluator(const Options &opts);
    explicit SumEvaluator(const std::vector<ScalarEvaluator *> &evals);
    virtual ~SumEvaluator() override;
};

#endif
