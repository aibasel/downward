#ifndef EVALUATORS_CONST_EVALUATOR_H
#define EVALUATORS_CONST_EVALUATOR_H

#include "../evaluator.h"

namespace plugins {
class Options;
}

namespace const_evaluator {
class ConstEvaluator : public Evaluator {
    int value;

protected:
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

public:
    ConstEvaluator(
        const std::shared_ptr<AbstractTask> &task, int value,
        const std::string &description, utils::Verbosity verbosity);
    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &) override {
    }
};
}

#endif
