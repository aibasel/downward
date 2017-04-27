#ifndef EVALUATION_CONTEXT_H
#define EVALUATION_CONTEXT_H

#include "evaluation_result.h"
#include "heuristic_cache.h"
#include "operator_id.h"

#include <unordered_map>

class Evaluator;
class GlobalState;
class SearchStatistics;

/*
  TODO: Now that we have an explicit EvaluationResult class, it's
  perhaps not such a great idea to duplicate all its access methods
  like "get_heuristic_value()" etc. on EvaluationContext. Might be a
  simpler interface to just give EvaluationContext an operator[]
  method or other simple way of accessing a given EvaluationResult
  and then use the methods of the result directly.
*/

/*
  TODO/NOTE: The code currently uses "Evaluator" everywhere, but
  this should eventually be replaced by "Heuristic" once these are
  unified.
*/

/*
  EvaluationContext has two main purposes:

  1. It packages up the information that heuristics and open lists
     need in order to perform an evaluation: the state, the g value of
     the node, and whether it was reached by a preferred operator.

  2. It caches computed heuristic values and preferred operators for
     the current evaluation so that they do not need to be computed
     multiple times just because they appear in multiple contexts,
     and also so that we don't need to know a priori which heuristics
     need to be evaluated throughout the evaluation process.

     For example, our current implementation of A* search uses the
     heuristic value h at least three times: twice for its
     tie-breaking open list based on <g + h, h> and a third time for
     its "progress evaluator" that produces output whenever we reach a
     new best f value.
*/

class EvaluationContext {
    HeuristicCache cache;
    int g_value;
    bool preferred;
    SearchStatistics *statistics;
    bool calculate_preferred;

    static const int INVALID = -1;

public:
    /*
      Copy existing heuristic cache and use it to look up heuristic values.
      Used for example by lazy search.

      TODO: Can we reuse caches? Can we move them instead of copying them?
    */
    EvaluationContext(
        const HeuristicCache &cache, int g_value, bool is_preferred,
        SearchStatistics *statistics, bool calculate_preferred = false);
    /*
      Create new heuristic cache for caching heuristic values. Used for example
      by eager search.
    */
    EvaluationContext(
        const GlobalState &state, int g_value, bool is_preferred,
        SearchStatistics *statistics, bool calculate_preferred = false);
    /*
      Use the following constructor when you don't care about g values,
      preferredness (and statistics), e.g. when sampling states for heuristics.

      This constructor sets g_value to -1 and checks that neither get_g_value()
      nor is_preferred() are called for objects constructed with it.

      TODO: In the long term we might want to separate how path-dependent and
            path-independent heuristics are evaluated. This change would remove
            the need to store the g value and preferredness for evaluation
            contexts that don't need this information.
    */
    EvaluationContext(
        const GlobalState &state,
        SearchStatistics *statistics = nullptr, bool calculate_preferred = false);

    ~EvaluationContext() = default;

    const EvaluationResult &get_result(Evaluator *heur);
    const HeuristicCache &get_cache() const;
    const GlobalState &get_state() const;
    int get_g_value() const;
    bool is_preferred() const;

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
    bool is_heuristic_infinite(Evaluator *heur);
    int get_heuristic_value(Evaluator *heur);
    int get_heuristic_value_or_infinity(Evaluator *heur);
    const std::vector<OperatorID> &get_preferred_operators(
        Evaluator *heur);
    bool get_calculate_preferred() const;
};

#endif
