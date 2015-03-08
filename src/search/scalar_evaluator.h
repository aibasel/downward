#ifndef SCALAR_EVALUATOR_H
#define SCALAR_EVALUATOR_H

#include "evaluation_result.h"

#include <set>

class EvaluationContext;
class Heuristic;

class ScalarEvaluator {
public:
    virtual ~ScalarEvaluator() {}

    virtual bool dead_end_is_reliable() const = 0;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) = 0;

    /*
      We should make sure that evaluators don't directly call
      compute_result for their subevaluators, as this could circumvent
      the caching mechanism provided by the EvaluationContext. The
      compute_result method should only be called by EvaluationContext.

      TODO: Think of a clean way to achieve that.
    */
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) = 0;
};

#endif
