#ifndef EVALUATOR_CACHE_H
#define EVALUATOR_CACHE_H

#include "evaluation_result.h"

#include <unordered_map>

class TaskSpecificEvaluator;

using EvaluationResults = std::unordered_map<TaskSpecificEvaluator *, EvaluationResult>;

/*
  Store evaluation results for evaluators.
*/
class EvaluatorCache {
    EvaluationResults eval_results;

public:
    EvaluationResult &operator[](TaskSpecificEvaluator *eval);

    template<class Callback>
    void for_each_evaluator_result(const Callback &callback) const {
        for (const auto &element : eval_results) {
            const TaskSpecificEvaluator *eval = element.first;
            const EvaluationResult &result = element.second;
            callback(eval, result);
        }
    }
};

#endif
