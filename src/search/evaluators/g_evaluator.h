#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../evaluator.h"

namespace g_evaluator {
class GEvaluator : public Evaluator {
public:
    explicit GEvaluator(const plugins::Options &opts); // TODO issue1082: remove this
    GEvaluator(
        bool use_for_reporting_minima,
        bool use_for_boosting,
        bool use_for_counting_evaluations,
        const std::string &description,
        utils::Verbosity verbosity);
    
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &) override {}
};
}

#endif
