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
    explicit SumEvaluator(const plugins::Options &opts); // TODO issue1082 remove this
    SumEvaluator(
        std::vector<std::shared_ptr<Evaluator>> evals,
        bool use_for_reporting_minima,
        bool use_for_boosting,
        bool use_for_counting_evaluations,
        const std::string &description,
        utils::Verbosity verbosity);
};
}

#endif
