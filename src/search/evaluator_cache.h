#ifndef EVALUATOR_CACHE_H
#define EVALUATOR_CACHE_H

#include "evaluation_result.h"
#include "global_state.h"

#include <unordered_map>

class Evaluator;

using EvaluationResults = std::unordered_map<Evaluator *, EvaluationResult>;

/*
  Store a state and evaluation results for this state.
*/
class EvaluatorCache {
    EvaluationResults eval_results;
    GlobalState state;

public:
    explicit EvaluatorCache(const GlobalState &state);
    ~EvaluatorCache() = default;

    EvaluationResult &operator[](Evaluator *eval);

    const GlobalState &get_state() const;

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
