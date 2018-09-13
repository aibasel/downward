#include "heuristic_cache.h"

using namespace std;


HeuristicCache::HeuristicCache(const GlobalState &state)
    : state(state) {
}

EvaluationResult &HeuristicCache::operator[](Evaluator *eval) {
    return eval_results[eval];
}

const GlobalState &HeuristicCache::get_state() const {
    return state;
}
