#ifndef EVALUATORS_STATE_FORWARDING_EVALUATOR_H
#define EVALUATORS_STATE_FORWARDING_EVALUATOR_H

#include "../evaluator.h"
#include "../state_registry.h"

#include <memory.h>
#include <unordered_map>

namespace state_forwarding_evaluator {
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
class StateForwardingEvaluator : public Evaluator {
    const std::shared_ptr<AbstractTask> transformed_task;
    std::shared_ptr<Evaluator> nested;
    mutable std::unordered_map<
        const StateRegistry *, std::unique_ptr<DelegatingStateRegistry>>
        state_registries;
    mutable const StateRegistry *last_state_registry_key;
    mutable DelegatingStateRegistry *last_state_registry;
    std::vector<Evaluator *> path_dependent_evaluators;
    State convert_state(const State &state) const;
public:
    StateForwardingEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::shared_ptr<AbstractTask> &transformed_task,
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
