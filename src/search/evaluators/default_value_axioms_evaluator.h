#ifndef EVALUATORS_AXIOM_HANDLING_EVALUATOR_H
#define EVALUATORS_AXIOM_HANDLING_EVALUATOR_H

#include "../component.h"
#include "../evaluator.h"
#include "../state_registry.h"

#include "../tasks/default_value_axioms_task.h"

#include <memory.h>

namespace axiom_handling_evaluator {
class AxiomHandlingEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> nested;
    mutable std::unique_ptr<DelegatingStateRegistry> state_registry;
    State convert_state(const State &state) const;
public:
    AxiomHandlingEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::shared_ptr<Evaluator> &nested, bool use_for_reporting_minima,
        bool use_for_boosting, bool use_for_counting_evaluations,
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

class TaskIndependentAxiomHandlingEvaluator
    : public TaskIndependentEvaluator {
    std::shared_ptr<TaskIndependentEvaluator> nested;
    tasks::AxiomHandlingType axioms;
    bool use_for_reporting_minima;
    bool use_for_boosting;
    bool use_for_counting_evaluations;
    const std::string description;
    utils::Verbosity verbosity;

    virtual std::shared_ptr<Evaluator> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task) const override;
public:
    TaskIndependentAxiomHandlingEvaluator(
        std::shared_ptr<TaskIndependentEvaluator> nested,
        tasks::AxiomHandlingType axioms, bool use_for_reporting_minima,
        bool use_for_boosting, bool use_for_counting_evaluations,
        const std::string &description, utils::Verbosity verbosity);
};

std::shared_ptr<TaskIndependentEvaluator> wrap_in_axiom_handling_evaluator(
    const std::shared_ptr<TaskIndependentEvaluator> &eval,
    const plugins::Options &opts);
}

#endif
