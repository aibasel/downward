#include "node_evaluation_context.h"

#include "evaluation_result.h"

using namespace std;


NodeEvaluationContext::NodeEvaluationContext(
    const StateEvaluationContext &eval_context, int g_value, bool is_preferred)
    : eval_context(eval_context),
      g_value(g_value),
      preferred(is_preferred) {
}

const EvaluationResult &NodeEvaluationContext::get_result(ScalarEvaluator *heur) {
    return eval_context.get_result(heur);
}

const GlobalState &NodeEvaluationContext::get_state() const {
    return eval_context.get_state();
}

int NodeEvaluationContext::get_g_value() const {
    return g_value;
}

bool NodeEvaluationContext::is_preferred() const {
    return preferred;
}
