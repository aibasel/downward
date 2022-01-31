#include "evaluator_cache.h"

using namespace std;


EvaluationResult &EvaluatorCache::operator[](Evaluator *eval) {
    return eval_results[eval];
}
