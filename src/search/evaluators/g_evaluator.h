#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../evaluator.h"
#include "../per_state_information.h"

#include <memory>

namespace g_evaluator {
class GEvaluator : public Evaluator {
    std::shared_ptr<AbstractTask> task;
    OperatorsProxy operators;
    PerStateInformation<int> g_values;
public:
    GEvaluator(const std::shared_ptr<AbstractTask> &task);

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &evals) override;

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(
        const State &parent_state, OperatorID op_id, const State &state) override;
};
}

#endif
