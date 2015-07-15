#ifndef HEURISTIC_CACHE_H
#define HEURISTIC_CACHE_H

#include "evaluation_result.h"
#include "global_state.h"
#include "heuristic.h"

#include <unordered_map>

class ScalarEvaluator;

using EvaluationResults = std::unordered_map<ScalarEvaluator *, EvaluationResult>;

/*
  Store a state and evaluation results for this state.
*/
class HeuristicCache {
    EvaluationResults eval_results;
    GlobalState state;

public:
    explicit HeuristicCache(const GlobalState &state);
    ~HeuristicCache() = default;

    EvaluationResult &operator[](ScalarEvaluator *heur);

    const GlobalState &get_state() const;

    template<class Callback>
    void for_each_heuristic_value(const Callback &callback) const {
        for (const auto &element : eval_results) {
            const ScalarEvaluator *eval = element.first;
            const EvaluationResult &result = element.second;
            const Heuristic *heuristic = dynamic_cast<const Heuristic *>(eval);
            if (heuristic) {
                /* We want to consider only Heuristic instances, not other
                   ScalarEvaluator instances. */
                callback(heuristic, result);
            }
        }
    }
};

#endif
