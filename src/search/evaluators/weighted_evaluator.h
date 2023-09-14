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
    int weight;

public:
    explicit WeightedEvaluator(const plugins::Options &opts);
    explicit WeightedEvaluator(utils::LogProxy log,
                               std::shared_ptr<Evaluator> evaluator,
                               int weight,
                               std::basic_string<char> unparsed_config,
                               bool use_for_reporting_minima,
                               bool use_for_boosting,
                               bool use_for_counting_evaluations,
                               std::vector<std::shared_ptr<Evaluator>> subevaluators);
    virtual ~WeightedEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;
};
}

#endif
