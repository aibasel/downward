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
    explicit CombiningEvaluator(const plugins::Options &opts);
    explicit CombiningEvaluator(utils::LogProxy log,
                                std::vector<std::shared_ptr<Evaluator>> subevaluators,
                                std::basic_string<char> unparsed_config = std::string(),
                                bool use_for_reporting_minima = false,
                                bool use_for_boosting = false,
                                bool use_for_counting_evaluations = false);
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

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override;
};

extern void add_combining_evaluator_options_to_feature(
    plugins::Feature &feature);



class TaskIndependentCombiningEvaluator : public TaskIndependentEvaluator {
protected:
    std::vector<std::shared_ptr<TaskIndependentEvaluator>> subevaluators;
public:
    explicit TaskIndependentCombiningEvaluator(utils::LogProxy log,
                                               std::vector<std::shared_ptr<TaskIndependentEvaluator>> subevaluators,
                                               std::basic_string<char> unparsed_config = std::string(),
                                               bool use_for_reporting_minima = false,
                                               bool use_for_boosting = false,
                                               bool use_for_counting_evaluations = false);
    virtual ~TaskIndependentCombiningEvaluator() = default;

    virtual std::shared_ptr<Evaluator> create_task_specific_Evaluator(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<CombiningEvaluator> create_task_specific_CombiningEvaluator(const std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<CombiningEvaluator> create_task_specific_CombiningEvaluator(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map, int depth = -1);
};
}
#endif
