#ifndef HEURISTIC_CACHE_H
#define HEURISTIC_CACHE_H

#include "evaluation_result.h"
#include "global_state.h"

#include <unordered_map>

class ScalarEvaluator;
class SearchStatistics;

using EvaluationResults = std::unordered_map<ScalarEvaluator *, EvaluationResult>;

class HeuristicCache {
    EvaluationResults eval_results;
    GlobalState state;
    SearchStatistics *statistics;

public:
    // TODO: Select one constructor?
    // Use "statistics = nullptr" if no statistics are desired.
    HeuristicCache(
        const GlobalState &state, SearchStatistics *statistics);

    /*
      Use the following constructor when you don't need statistics,
      e.g. when sampling states for heuristics.
    */
    explicit HeuristicCache(const GlobalState &state);
    ~HeuristicCache() = default;

    const EvaluationResults &get_eval_results() const {
        return eval_results;
    }

    const EvaluationResult &get_result(ScalarEvaluator *heur);

    const GlobalState &get_state() const;

    /*
      Use get_heuristic_value() to query finite heuristic values. It
      is an error (guarded by an assertion) to call this method for
      states with infinite heuristic values, because such states often
      need to be treated specially and we want to catch cases where we
      forget to do this.

      In cases where finite and infinite heuristic values can be
      treated uniformly, use get_heuristic_value_or_infinity(), which
      returns numeric_limits<int>::max() for infinite estimates.
    */
    bool is_heuristic_infinite(ScalarEvaluator *heur);
    int get_heuristic_value(ScalarEvaluator *heur);
    int get_heuristic_value_or_infinity(ScalarEvaluator *heur);
    const std::vector<const GlobalOperator *> &get_preferred_operators(
        ScalarEvaluator *heur);

    template<class Callback>
    void for_each_evaluator_value(const Callback &callback) const {
        for (const auto &element : get_eval_results()) {
            const ScalarEvaluator *eval = element.first;
            int h = element.second.get_h_value();
            callback(eval, h);
        }
    }
};

#endif
