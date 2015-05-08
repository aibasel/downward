#include "lazy_evaluation_context.h"

#include "evaluation_result.h"

using namespace std;


LazyEvaluationContext::LazyEvaluationContext(
    const EagerEvaluationContext &eval_context, int g_value, bool is_preferred)
    : eval_context(eval_context),
      g_value(g_value),
      preferred(is_preferred) {
}

const EvaluationResult &LazyEvaluationContext::get_result(ScalarEvaluator *heur) {
    return eval_context.get_result(heur);
}

const GlobalState &LazyEvaluationContext::get_state() const {
    return eval_context.get_state();
}

int LazyEvaluationContext::get_g_value() const {
    return g_value;
}

bool LazyEvaluationContext::is_preferred() const {
    return preferred;
}
