#ifndef NODE_EVALUATION_CONTEXT_H
#define NODE_EVALUATION_CONTEXT_H

#include "state_evaluation_context.h"

class NodeEvaluationContext {
    StateEvaluationContext eval_context;
    int g_value;
    bool preferred;

public:
    NodeEvaluationContext(
        const StateEvaluationContext &eval_context, int g_value, bool is_preferred);
    ~NodeEvaluationContext() = default;

    const EvaluationResults &get_eval_results() const {
        return eval_context.get_eval_results();
    }

    const EvaluationResult &get_result(ScalarEvaluator *heur);

    const GlobalState &get_state() const;

    int get_g_value() const;
    bool is_preferred() const;
};

#endif
