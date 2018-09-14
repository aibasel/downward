#include "evaluator_cache.h"

using namespace std;


EvaluatorCache::EvaluatorCache(const GlobalState &state)
    : state(state) {
}

EvaluationResult &EvaluatorCache::operator[](Evaluator *eval) {
    return eval_results[eval];
}

const GlobalState &EvaluatorCache::get_state() const {
    return state;
}
