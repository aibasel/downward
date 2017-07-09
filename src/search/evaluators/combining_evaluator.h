#ifndef EVALUATORS_COMBINING_EVALUATOR_H
#define EVALUATORS_COMBINING_EVALUATOR_H

#include "../evaluator.h"

#include <set>
#include <string>
#include <vector>

namespace combining_evaluator {
/*
  CombiningEvaluator is the base class for SumEvaluator and
  MaxEvaluator, which captures the common aspects of their behaviour.
*/
class CombiningEvaluator : public Evaluator {
    std::vector<Evaluator *> subevaluators;
    bool all_dead_ends_are_reliable;
protected:
    virtual int combine_values(const std::vector<int> &values) = 0;
public:
    explicit CombiningEvaluator(
        const std::vector<Evaluator *> &subevaluators_);
    virtual ~CombiningEvaluator() override;

    /*
      Note: dead_ends_are_reliable() is a state-independent method, so
      it only returns true if all subevaluators report dead ends reliably.

      Note that we could get more fine-grained information when
      considering of reliability for a given evaluated state. For
      example, if we use h1 (unreliable) and h2 (reliable) and have a
      state where h1 is finite and h2 is infinite, then we can
      *reliably* mark the state as a dead end. There is currently no
      way to exploit such state-based information, and hence we do not
      compute it.
    */

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;
};
}

#endif
