#ifndef EVALUATORS_WEIGHTED_EVALUATOR_H
#define EVALUATORS_WEIGHTED_EVALUATOR_H

#include "../evaluator.h"

#include <memory>

namespace plugins {
class Options;
}

namespace weighted_evaluator {
class WeightedEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> evaluator;
    int w;

public:
    explicit WeightedEvaluator(const plugins::Options &opts); // TODO issue1082 remove this
    WeightedEvaluator(
        std::shared_ptr<Evaluator> eval,
        int weight,
        bool use_for_reporting_minima,
        bool use_for_boosting,
        bool use_for_counting_evaluations,
        const std::string &description,
        utils::Verbosity verbosity);

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};
}

#endif
