#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../evaluator.h"

namespace g_evaluator {
using GEvaluatorArgs = std::tuple<std::string, utils::Verbosity>;
class GEvaluator : public Evaluator {
public:
    GEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::string &description, utils::Verbosity verbosity);

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &) override {
    }
};
}

#endif
