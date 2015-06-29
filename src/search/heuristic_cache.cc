#include "heuristic_cache.h"

using namespace std;


HeuristicCache::HeuristicCache(const GlobalState &state)
    : state(state) {
}

const EvaluationResults &HeuristicCache::get_eval_results() const {
    return eval_results;
}

EvaluationResult &HeuristicCache::operator[](ScalarEvaluator *heur) {
    return eval_results[heur];
}

const GlobalState &HeuristicCache::get_state() const {
    return state;
}
