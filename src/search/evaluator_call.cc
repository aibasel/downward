#include "evaluator_call.h"

#include "evaluation_context.h"
#include "evaluation_result.h"
#include "evaluator.h"
#include "evaluator_cache.h"
#include "search_statistics.h"
#include "task_proxy.h"

EvaluatorCall::EvaluatorCall(EvaluationContext &context, const State &state)
    : context(context), state(state) {
}

const EvaluationResult &EvaluatorCall::operator[](Evaluator *evaluator) {
    EvaluationResult &result = context.get_cache()[evaluator];
    if (result.is_uninitialized()) {
        SearchStatistics *statistics = context.get_statistics();
        result = evaluator->compute_result(*this);
        if (statistics && evaluator->is_used_for_counting_evaluations() &&
            result.get_count_evaluation()) {
            statistics->inc_evaluations();
        }
    }
    return result;
}

EvaluatorCall EvaluatorCall::get_subcall(const State &state) const {
    return EvaluatorCall(context, state);
}

const State &EvaluatorCall::get_state() const {
    return state;
}

int EvaluatorCall::get_g_value() const {
    return context.get_g_value();
}

bool EvaluatorCall::get_calculate_preferred() const {
    return context.get_calculate_preferred();
}

bool EvaluatorCall::is_preferred() const {
    return context.is_preferred();
}
