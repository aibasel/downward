#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "evaluation_result.h"

#include <set>

class EvaluationContext;
class GlobalState;

class Evaluator {
public:
    Evaluator() = default;
    virtual ~Evaluator() = default;

    /*
      dead_ends_are_reliable should return true if the evaluator is
      "safe", i.e., infinite estimates can be trusted.

      The default implementation returns true.
    */
    virtual bool dead_ends_are_reliable() const;

    /*
      get_path_dependent_evaluators should insert all path-dependent evaluators
      that this evaluator directly or indirectly depends on into the result set,
      including itself if necessary.

      TODO: We wanted to get rid of this at some point, and perhaps we
      still should try to do that. Currently, the only legitimate use
      for this seems to be to call "notify_state_transition" on all heuristics.
      (There is also one "illegitimate" use, the remaining reference
      to heuristics[0] in EagerSearch.)
    */
    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) = 0;


    virtual void notify_initial_state(const GlobalState & /*initial_state*/) {
    }

    /*
      TODO: I don't think we use the return value any more, in which
      case we should make this void.
    */
    virtual bool notify_state_transition(
            const GlobalState & /*parent_state*/,
            OperatorID /*op_id*/,
            const GlobalState & /*state*/) {
        return false;
    }

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
