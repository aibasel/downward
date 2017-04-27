#include "evaluation_context.h"

#include "evaluation_result.h"
#include "heuristic.h"
#include "search_statistics.h"

#include <cassert>

using namespace std;


EvaluationContext::EvaluationContext(
    const HeuristicCache &cache, int g_value, bool is_preferred,
    SearchStatistics *statistics, bool calculate_preferred)
    : cache(cache),
      g_value(g_value),
      preferred(is_preferred),
      statistics(statistics),
      calculate_preferred(calculate_preferred) {
}

EvaluationContext::EvaluationContext(
    const GlobalState &state, int g_value, bool is_preferred,
    SearchStatistics *statistics, bool calculate_preferred)
    : EvaluationContext(HeuristicCache(state), g_value, is_preferred, statistics, calculate_preferred) {
}

EvaluationContext::EvaluationContext(
    const GlobalState &state,
    SearchStatistics *statistics, bool calculate_preferred)
    : EvaluationContext(HeuristicCache(state), INVALID, false, statistics, calculate_preferred) {
}

const EvaluationResult &EvaluationContext::get_result(Evaluator *heur) {
    EvaluationResult &result = cache[heur];
    if (result.is_uninitialized()) {
        result = heur->compute_result(*this);
        if (statistics && dynamic_cast<const Heuristic *>(heur)) {
            /* Only count evaluations of actual Heuristics, not arbitrary
               evaluators. */
            if (result.get_count_evaluation()) {
                statistics->inc_evaluations();
            }
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

bool EvaluationContext::is_heuristic_infinite(Evaluator *heur) {
    return get_result(heur).is_infinite();
}

int EvaluationContext::get_heuristic_value(Evaluator *heur) {
    int h = get_result(heur).get_h_value();
    assert(h != EvaluationResult::INFTY);
    return h;
}

int EvaluationContext::get_heuristic_value_or_infinity(Evaluator *heur) {
    return get_result(heur).get_h_value();
}

const vector<OperatorID> &
EvaluationContext::get_preferred_operators(Evaluator *heur) {
    return get_result(heur).get_preferred_operators();
}


bool EvaluationContext::get_calculate_preferred() const {
    return calculate_preferred;
}
