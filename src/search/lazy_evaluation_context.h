#ifndef LAZY_EVALUATION_CONTEXT_H
#define LAZY_EVALUATION_CONTEXT_H

#include "eager_evaluation_context.h"

class LazyEvaluationContext : public EvaluationContext {
    EagerEvaluationContext eval_context;

public:
    LazyEvaluationContext(const EagerEvaluationContext &eval_context);
    ~LazyEvaluationContext() = default;

    virtual const EvaluationResult &get_result(ScalarEvaluator *heur) override;

    virtual const GlobalState &get_state() const override;

    virtual int get_g_value() const override;
    virtual bool is_preferred() const override;
};

#endif
