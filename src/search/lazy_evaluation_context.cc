#include "lazy_evaluation_context.h"

#include "evaluation_result.h"

using namespace std;


LazyEvaluationContext::LazyEvaluationContext(
    const EagerEvaluationContext &eval_context)
    : eval_context(eval_context) {
}

const EvaluationResult &LazyEvaluationContext::get_result(ScalarEvaluator *heur) {
    return eval_context.get_result(heur);
}

const GlobalState &LazyEvaluationContext::get_state() const {
    return eval_context.get_state();
}

int LazyEvaluationContext::get_g_value() const {
    return eval_context.get_g_value();
}

bool LazyEvaluationContext::is_preferred() const {
    return eval_context.is_preferred();
}
