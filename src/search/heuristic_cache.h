#ifndef HEURISTIC_CACHE_H
#define HEURISTIC_CACHE_H

#include "evaluation_result.h"
#include "global_state.h"

#include <unordered_map>

class ScalarEvaluator;

using EvaluationResults = std::unordered_map<ScalarEvaluator *, EvaluationResult>;

class HeuristicCache {
    EvaluationResults eval_results;
    GlobalState state;

public:
    explicit HeuristicCache(const GlobalState &state);
    ~HeuristicCache() = default;

    const EvaluationResults &get_eval_results() const {
        return eval_results;
    }

    EvaluationResult &get_result(ScalarEvaluator *heur);

    const GlobalState &get_state() const;
};

#endif
