#ifndef EVALUATION_CONTEXT_H
#define EVALUATION_CONTEXT_H

#include "global_state.h"

#include <limits>
#include <unordered_map>

class GlobalOperator;
class Heuristic;

class EvaluationContext {
    static const int INFINITE = std::numeric_limits<int>::max();
    static const int UNINITIALIZED = -2;

    struct HeuristicResult {
        int h_value;
        std::vector<const GlobalOperator *> preferred_operators;

        HeuristicResult() : h_value(UNINITIALIZED) {
        }

        bool is_uninitialized() const {
            return h_value == UNINITIALIZED;
        }

        bool is_dead_end() const {
            return h_value == INFINITE;
        }
    };

    GlobalState state;
    std::unordered_map<Heuristic *, HeuristicResult> heuristic_results;

    const HeuristicResult &get_result(Heuristic *heur);
public:
    EvaluationContext(const GlobalState &state);

    /*
      Use get_heuristic_value() to query finite heuristic value. It is
      an error to call this method for dead-ends, and this is checked
      by an assertion.

      In cases where dead-ends can be treated uniformly with finite
      heuristic values, use get_heuristic_value_or_infinity(), which
      returns numeric_limits<int>::max() for infinite estimates.
    */
    bool is_dead_end(Heuristic *heur);
    int get_heuristic_value(Heuristic *heur);
    int get_heuristic_value_or_infinity(Heuristic *heur);
    const std::vector<const GlobalOperator *> &get_preferred_operators(
        Heuristic *heur);
};

#endif
