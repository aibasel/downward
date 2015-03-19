#include "evaluation_context.h"

#include "evaluation_result.h"
#include "heuristic.h"
#include "scalar_evaluator.h"
#include "search_statistics.h"

#include <cassert>

using namespace std;


EvaluationContext::EvaluationContext(
    const GlobalState &state, int g_value, bool is_preferred,
    SearchStatistics *statistics)
    : state(state),
      g_value(g_value),
      preferred(is_preferred),
      statistics(statistics) {
}

EvaluationContext::EvaluationContext(const GlobalState &state)
    : EvaluationContext(state, 0, false, nullptr) {
}

const EvaluationResult &EvaluationContext::get_result(ScalarEvaluator *heur) {
    EvaluationResult &result = eval_results[heur];
    if (result.is_uninitialized()) {
        result = heur->compute_result(*this);
        if (statistics && dynamic_cast<const Heuristic *>(heur)) {
            /* Only count evaluations of actual Heuristics, not arbitrary
               scalar evaluators. */
            statistics->inc_evaluations();
        }
    }
    return result;
}

const GlobalState &EvaluationContext::get_state() const {
    return state;
}

int EvaluationContext::get_g_value() const {
    return g_value;
}

bool EvaluationContext::is_preferred() const {
    return preferred;
}

bool EvaluationContext::is_heuristic_infinite(ScalarEvaluator *heur) {
    return get_result(heur).is_infinite();
}

int EvaluationContext::get_heuristic_value(ScalarEvaluator *heur) {
    int h = get_result(heur).get_h_value();
    assert(h != EvaluationResult::INFINITE);
    return h;
}

int EvaluationContext::get_heuristic_value_or_infinity(ScalarEvaluator *heur) {
    return get_result(heur).get_h_value();
}

const vector<const GlobalOperator *> &
EvaluationContext::get_preferred_operators(ScalarEvaluator *heur) {
    return get_result(heur).get_preferred_operators();
}
