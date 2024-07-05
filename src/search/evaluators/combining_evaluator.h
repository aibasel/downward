#ifndef EVALUATORS_COMBINING_EVALUATOR_H
#define EVALUATORS_COMBINING_EVALUATOR_H

#include "../evaluator.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace combining_evaluator {
/*
  CombiningEvaluator is the base class for SumEvaluator and
  MaxEvaluator, which captures the common aspects of their behaviour.
*/
class CombiningEvaluator : public Evaluator {
    std::vector<std::shared_ptr<Evaluator>> subevaluators;
    bool all_dead_ends_are_reliable;
protected:
    virtual int combine_values(const std::vector<int> &values) = 0;
public:
    CombiningEvaluator(
        const std::vector<std::shared_ptr<Evaluator>> &evals,
        const std::string &description, utils::Verbosity verbosity);

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

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override;
};

extern void add_combining_evaluator_options_to_feature(
    plugins::Feature &feature, const std::string &description);
extern std::tuple<std::vector<std::shared_ptr<Evaluator>>,
                  const std::string, utils::Verbosity>
get_combining_evaluator_arguments_from_options(
    const plugins::Options &opts);
}

#endif
