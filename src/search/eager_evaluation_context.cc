#include "eager_evaluation_context.h"

#include "evaluation_result.h"
#include "heuristic.h"
#include "search_statistics.h"

using namespace std;


EagerEvaluationContext::EagerEvaluationContext(
    const GlobalState &state, int g_value, bool is_preferred,
    SearchStatistics *statistics)
    : state(state),
      g_value(g_value),
      preferred(is_preferred),
      statistics(statistics) {
}

EagerEvaluationContext::EagerEvaluationContext(const GlobalState &state)
    : EagerEvaluationContext(state, 0, false, nullptr) {
}

const EvaluationResult &EagerEvaluationContext::get_result(ScalarEvaluator *heur) {
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

const GlobalState &EagerEvaluationContext::get_state() const {
    return state;
}

int EagerEvaluationContext::get_g_value() const {
    return g_value;
}

bool EagerEvaluationContext::is_preferred() const {
    return preferred;
}
