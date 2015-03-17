#ifndef EVALUATION_CONTEXT_H
#define EVALUATION_CONTEXT_H

#include "evaluation_result.h"
#include "global_state.h"

#include <limits>
#include <unordered_map>

class GlobalOperator;
class ScalarEvaluator;

/*
  TODO: Now that we have an explicit EvaluationResult class, it's
  perhaps not such a great idea to duplicate all its access methods
  like "get_heuristic_value()" etc. on EvaluationContext. Might be a
  simpler interface to just give EvaluationContext an operator[]
  method or other simple way of accessing a given EvaluationResult
  and then use the methods of the result directly.
*/

/*
  TODO/NOTE: The code currently uses "ScalarEvaluator" everywhere, but
  this should eventually be replaced by "Heuristic" once these are
  unified.
*/

/*
  TODO/NOTE: I'm a bit conflicted how to use the EvaluationContext
  class at the moment. The idea is that it serves as a kind of cache
  for heuristic values in a given state that allows us to (eventually)
  get rid of the need to store information about "the current state"
  inside the heuristics and open lists, which was always a bit
  fragile.

  We want to maintain the property that we only compute each heuristic
  value once per state evaluation, and the current implementation
  achieves this by only actually computing the heuristic value the
  first time we ask for it, using a cached value in subsequent
  queries. For example, in an A* search, we generally use the
  heuristic value h twice because it's a tie-breaking open list using
  <g + h, h>, but we only want to compute h once.

  I thought that given such a cache, we might be able to do away with
  the current model where we determine the "involved heuristics" up
  front and then compute the heuristic values prior to use -- we could
  instead compute them on demand. But this clashes with a few features
  where we currently want to have access to "all involved heuristics"
  up front during an evaluation, namely

  - to determine if we want to consider the current state as a
    dead-end state
  - for statistical output

  This is why EvaluationContext grew an "evaluate_heuristic" method
  for now, which populates the cache without the user actually using
  this specific heuristic value right now: this is necessary to make
  the heuristic value available for methods that use all heuristic
  information like "EvaluationContext::is_dead_end().

      Later update: EvaluationContext::is_dead_end() is gone now, so
      we're now quite some ways towards rectifying this and getting
      rid of this need to precompute.

  But this means that it is easy to forget to call evaluate_heuristic
  without noticing that we lose dead-end pruning power, so it may
  ultimately be a better idea to *require* that evaluate_heuristic is
  called before we query values for a given heuristic.

  I'm leaving this as is for now because I'd like to focus on other
  critical bits of the implementation first, but the question of
  whether or not to use up-front computation of the heuristics and
  whether to automatically add them later when querying a heuristic
  value that isn't known yet needs to be revisited eventually.
*/

class EvaluationContext {
    GlobalState state;
    int g_value;
    bool preferred;
    std::unordered_map<ScalarEvaluator *, EvaluationResult> eval_results;

    const EvaluationResult &get_result(ScalarEvaluator *heur);
public:
    EvaluationContext(const GlobalState &state, int g, bool preferred);

    const GlobalState &get_state() const;

    void evaluate_heuristic(ScalarEvaluator *heur);

    void hacky_set_evaluator_value(ScalarEvaluator *heur, int value);

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
    bool is_heuristic_infinite(ScalarEvaluator *heur);
    int get_heuristic_value(ScalarEvaluator *heur);
    int get_heuristic_value_or_infinity(ScalarEvaluator *heur);
    const std::vector<const GlobalOperator *> &get_preferred_operators(
        ScalarEvaluator *heur);
};

#endif
