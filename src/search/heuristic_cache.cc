#include "heuristic_cache.h"

using namespace std;


HeuristicCache::HeuristicCache(const GlobalState &state)
    : state(state) {
}

EvaluationResult &HeuristicCache::get_result(ScalarEvaluator *heur) {
    return eval_results[heur];
}

const GlobalState &HeuristicCache::get_state() const {
    return state;
}
