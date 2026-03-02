#ifndef EVALUATORS_COST_ADAPTED_EVALUATOR_H
#define EVALUATORS_COST_ADAPTED_EVALUATOR_H

#include "../component.h"
#include "../evaluator.h"
#include "../operator_cost.h"

#include <memory.h>

namespace cost_adapted_evaluator {
class ModifyCostsEvaluator : public Evaluator {
    std::shared_ptr<Evaluator> nested;
public:
    ModifyCostsEvaluator(
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

class TaskIndependentModifyCostsEvaluator : public TaskIndependentEvaluator {
    std::shared_ptr<TaskIndependentEvaluator> nested;
    OperatorCost cost_type;

    virtual std::shared_ptr<Evaluator> create_task_specific_component(
        const std::shared_ptr<AbstractTask> &task,
        components::Cache &cache) const override;
public:
    TaskIndependentModifyCostsEvaluator(
        std::shared_ptr<TaskIndependentEvaluator> nested,
        OperatorCost cost_type);
};
}

#endif
