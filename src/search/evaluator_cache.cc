#include "evaluator_cache.h"

using namespace std;

EvaluationResult &EvaluatorCache::operator[](TaskSpecificEvaluator *eval) {
    return eval_results[eval];
}
