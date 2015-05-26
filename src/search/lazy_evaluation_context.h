#ifndef LAZY_EVALUATION_CONTEXT_H
#define LAZY_EVALUATION_CONTEXT_H

#include "eager_evaluation_context.h"

class LazyEvaluationContext : public EvaluationContext {
    EagerEvaluationContext eval_context;
    int g_value;
    bool preferred;

public:
    LazyEvaluationContext(
        const EagerEvaluationContext &eval_context, int g_value, bool is_preferred);
    virtual ~LazyEvaluationContext() = default;

    virtual const EvaluationResults &get_eval_results() const {
        return eval_context.get_eval_results();
    }

    virtual const EvaluationResult &get_result(ScalarEvaluator *heur) override;

    virtual const GlobalState &get_state() const override;

    virtual int get_g_value() const override;
    virtual bool is_preferred() const override;
};

#endif
