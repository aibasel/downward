#ifndef EVALUATOR_CACHE_H
#define EVALUATOR_CACHE_H

#include "evaluation_result.h"

#include <unordered_map>

class Evaluator;

using EvaluationResults = std::unordered_map<Evaluator *, EvaluationResult>;

/*
  Store evaluation results for evaluators.
*/
class EvaluatorCache {
    EvaluationResults eval_results;

public:
    EvaluationResult &operator[](Evaluator *eval);

    template<class Callback>
    void for_each_evaluator_result(const Callback &callback) const {
        for (const auto &element : eval_results) {
            const Evaluator *eval = element.first;
            const EvaluationResult &result = element.second;
            callback(eval, result);
        }
    }
};

#endif
