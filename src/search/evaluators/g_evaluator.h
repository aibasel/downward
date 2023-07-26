#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../evaluator.h"

namespace g_evaluator {
class GEvaluator : public Evaluator {
public:
    explicit GEvaluator(const plugins::Options &opts);
    explicit GEvaluator(utils::LogProxy log,
                        std::basic_string<char> unparsed_config = std::string(),
                        bool use_for_reporting_minima = false,
                        bool use_for_boosting = false,
                        bool use_for_counting_evaluations = false);
    virtual ~GEvaluator() override = default;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &) override {}
};
}

#endif
