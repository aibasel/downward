#ifndef EVALUATORS_SIMPLE_TASK_TRANSFORMING_EVALUATOR_H
#define EVALUATORS_SIMPLE_TASK_TRANSFORMING_EVALUATOR_H

#include "../evaluator.h"
#include "../state_registry.h"

#include <memory.h>

namespace simple_task_transforming_evaluator {
/*
  This evaluator evaluates a nested evaluator on a transformed version of its
  own task. The assumption is that the nested evaluator uses a task that has
  the same set of states as the task used to instantiate the transforming
  evaluator. States are thus not translated to the nested task before passing
  them on, but instead only re-interpreted as states of the nested task without
  storing them in a dedicated state registry. They keep their state IDs but are
  passed on to the nested evaluator as states of the nested task, so the nested
  evaluator does not have to know about the task transformation.
*/
class SimpleTaskTransformingEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> nested;
    mutable std::unique_ptr<DelegatingStateRegistry> state_registry;
    State convert_state(const State &state) const;
public:
    SimpleTaskTransformingEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::shared_ptr<Evaluator> &nested,
        const std::string &description, utils::Verbosity verbosity);

    virtual bool dead_ends_are_reliable() const override;
    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override;
    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(
        const State &parent_state, OperatorID op_id,
        const State &state) override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual bool does_cache_estimates() const override;
    virtual bool is_estimate_cached(const State &state) const override;
    virtual int get_cached_estimate(const State &state) const override;
};
}

#endif
