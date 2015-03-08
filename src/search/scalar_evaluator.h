#ifndef SCALAR_EVALUATOR_H
#define SCALAR_EVALUATOR_H

#include "evaluation_result.h"
#include "evaluator.h"
#include "utilities.h"

class EvaluationContext;

class ScalarEvaluator : public Evaluator {
public:
    virtual ~ScalarEvaluator() {}

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) = 0;

    virtual bool is_dead_end() const override final {
        ABORT("ScalarEvaluator::is_dead_end() should disappear. Don't call it.");
    }

    virtual void evaluate(int /*g*/, bool /*preferred*/) override final {
        ABORT("ScalarEvaluator::evaluate() should disappear. Don't call it.");
    }
};

#endif
