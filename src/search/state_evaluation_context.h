#ifndef STATE_EVALUATION_CONTEXT_H
#define STATE_EVALUATION_CONTEXT_H

#include "evaluation_result.h"
#include "global_state.h"

#include <unordered_map>

class ScalarEvaluator;
class SearchStatistics;

using EvaluationResults = std::unordered_map<ScalarEvaluator *, EvaluationResult>;

class StateEvaluationContext {
    EvaluationResults eval_results;
    GlobalState state;
    SearchStatistics *statistics;

public:
    // TODO: Select one constructor?
    // Use "statistics = nullptr" if no statistics are desired.
    StateEvaluationContext(
        const GlobalState &state, SearchStatistics *statistics);

    /*
      Use the following constructor when you don't need statistics,
      e.g. when sampling states for heuristics.
    */
    explicit StateEvaluationContext(const GlobalState &state);
    ~StateEvaluationContext() = default;

    const EvaluationResults &get_eval_results() const {
        return eval_results;
    }

    const EvaluationResult &get_result(ScalarEvaluator *heur);

    const GlobalState &get_state() const;
};

#endif
