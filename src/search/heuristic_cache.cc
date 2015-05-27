#include "heuristic_cache.h"

#include "evaluation_context.h"
#include "heuristic.h"
#include "search_statistics.h"

using namespace std;


HeuristicCache::HeuristicCache(
    const GlobalState &state, SearchStatistics *statistics)
    : state(state),
      statistics(statistics) {
}

HeuristicCache::HeuristicCache(const GlobalState &state)
    : HeuristicCache(state, nullptr) {
}

const EvaluationResult &HeuristicCache::get_result(ScalarEvaluator *heur) {
    assert(dynamic_cast<const Heuristic *>(heur));
    EvaluationResult &result = eval_results[heur];
    if (result.is_uninitialized()) {
        // TODO: Can we avoid using dummy values?
        EvaluationContext eval_context(state, -1, false, nullptr);
        result = heur->compute_result(eval_context);
        if (statistics) {
            /* Only count evaluations of actual Heuristics, not arbitrary
               scalar evaluators. */
            statistics->inc_evaluations();
        }
    }
    return result;
}

const GlobalState &HeuristicCache::get_state() const {
    return state;
}

bool HeuristicCache::is_heuristic_infinite(ScalarEvaluator *heur) {
    return get_result(heur).is_infinite();
}

int HeuristicCache::get_heuristic_value(ScalarEvaluator *heur) {
    int h = get_result(heur).get_h_value();
    assert(h != EvaluationResult::INFINITE);
    return h;
}

int HeuristicCache::get_heuristic_value_or_infinity(ScalarEvaluator *heur) {
    return get_result(heur).get_h_value();
}

const vector<const GlobalOperator *> &
HeuristicCache::get_preferred_operators(ScalarEvaluator *heur) {
    return get_result(heur).get_preferred_operators();
}
