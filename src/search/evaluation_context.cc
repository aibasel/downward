#include "evaluation_context.h"

#include "evaluation_result.h"
#include "heuristic.h"
#include "search_statistics.h"

#include <cassert>

using namespace std;


EvaluationContext::EvaluationContext(
    const HeuristicCache &cache, int g_value, bool is_preferred,
    SearchStatistics *statistics)
    : cache(cache),
      g_value(g_value),
      preferred(is_preferred),
      statistics(statistics) {
}

EvaluationContext::EvaluationContext(
    const GlobalState &state, int g_value, bool is_preferred,
    SearchStatistics *statistics)
    : EvaluationContext(HeuristicCache(state), g_value, is_preferred, statistics) {
}

EvaluationContext::EvaluationContext(
    const GlobalState &state,
    SearchStatistics *statistics)
    : EvaluationContext(HeuristicCache(state), INVALID, false, statistics) {
}

const EvaluationResult &EvaluationContext::get_result(ScalarEvaluator *heur) {
    EvaluationResult &result = cache[heur];
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

const HeuristicCache &EvaluationContext::get_cache() const {
    return cache;
}

const GlobalState &EvaluationContext::get_state() const {
    return cache.get_state();
}

int EvaluationContext::get_g_value() const {
    assert(g_value != INVALID);
    return g_value;
}

bool EvaluationContext::is_preferred() const {
    assert(g_value != INVALID);
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
