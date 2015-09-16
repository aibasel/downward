#ifndef SCALAR_EVALUATOR_H
#define SCALAR_EVALUATOR_H

#include "evaluation_result.h"

#include <set>

class EvaluationContext;
class Heuristic;

class ScalarEvaluator {
public:
    ScalarEvaluator() = default;
    virtual ~ScalarEvaluator() = default;

    /*
      dead_ends_are_reliable should return true if the evaluator is
      "safe", i.e., infinite estimates can be trusted.

      The default implementation returns true.
    */
    virtual bool dead_ends_are_reliable() const;

    /*
      get_involved_heuristics should insert all heuristics that this
      evaluator directly or indirectly depends on into the result set,
      including itself if it is a heuristic.
    */
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) = 0;

    /*
      compute_result should compute the estimate and possibly
      preferred operators for the given evaluation context and return
      them as an EvaluationResult instance.

      It should not add the result to the evaluation context -- this
      is done automatically elsewhere.

      The passed-in evaluation context is not const because this
      evaluator might depend on other evaluators, in which case their
      results will be stored in the evaluation context as a side
      effect (to make sure they are only computed once).

      TODO: We should make sure that evaluators don't directly call
      compute_result for their subevaluators, as this could circumvent
      the caching mechanism provided by the EvaluationContext. The
      compute_result method should only be called by
      EvaluationContext. We need to think of a clean way to achieve
      this.
    */
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) = 0;
};

#endif
