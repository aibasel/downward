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
    explicit SumEvaluator(std::basic_string<char> unparsed_config,
                          bool use_for_reporting_minima,
                          bool use_for_boosting,
                          bool use_for_counting_evaluations,
                          utils::LogProxy log,
                          std::vector<std::shared_ptr<Evaluator>> subevaluators);
    virtual ~SumEvaluator() override;
};
}

#endif
