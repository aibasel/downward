#include "state_evaluation_context.h"

#include "eager_evaluation_context.h"
#include "heuristic.h"
#include "search_statistics.h"

using namespace std;


StateEvaluationContext::StateEvaluationContext(
    const GlobalState &state, SearchStatistics *statistics)
    : state(state),
      statistics(statistics) {
}

StateEvaluationContext::StateEvaluationContext(const GlobalState &state)
    : StateEvaluationContext(state, nullptr) {
}

const EvaluationResult &StateEvaluationContext::get_result(ScalarEvaluator *heur) {
    EvaluationResult &result = eval_results[heur];
    if (result.is_uninitialized()) {
        // TODO: Fix this.
        EagerEvaluationContext eval_context(state, 0, false, nullptr);
        result = heur->compute_result(eval_context);
        if (statistics && dynamic_cast<const Heuristic *>(heur)) {
            /* Only count evaluations of actual Heuristics, not arbitrary
               scalar evaluators. */
            statistics->inc_evaluations();
        }
    }
    return result;
}

const GlobalState &StateEvaluationContext::get_state() const {
    return state;
}
