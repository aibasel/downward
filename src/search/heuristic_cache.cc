#include "heuristic_cache.h"

using namespace std;


HeuristicCache::HeuristicCache(const GlobalState &state)
    : state(state) {
}

bool HeuristicCache::empty() const {
    return eval_results.empty();
}

EvaluationResult &HeuristicCache::operator[](ScalarEvaluator *heur) {
    return eval_results[heur];
}

const GlobalState &HeuristicCache::get_state() const {
    return state;
}
