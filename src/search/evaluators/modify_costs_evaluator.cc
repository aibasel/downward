#include "modify_costs_evaluator.h"

#include "../plugins/plugin.h"
#include "../tasks/cost_adapted_task.h"

using namespace std;

namespace cost_adapted_evaluator {
shared_ptr<Evaluator>
TaskIndependentCostAdaptedEvaluator::create_task_specific_component(
    const shared_ptr<AbstractTask> &task, components::Cache &cache) const {
    shared_ptr<AbstractTask> cost_adapted_task =
        make_shared<tasks::CostAdaptedTask>(task, cost_type);
    return nested->bind_task(cost_adapted_task, cache);
}

TaskIndependentCostAdaptedEvaluator::TaskIndependentCostAdaptedEvaluator(
    shared_ptr<TaskIndependentEvaluator> nested, OperatorCost cost_type)
    : nested(nested), cost_type(cost_type) {
}

class CostAdaptedEvaluatorFeature
    : public plugins::TaskIndependentFeature<TaskIndependentEvaluator> {
public:
    CostAdaptedEvaluatorFeature() : TaskIndependentFeature("eval_modify_costs") {
        document_title("Cost-modifying evaluator");
        document_synopsis(
            "Evaluates the nested evaluator on a task with a modified cost"
            "function");

        add_option<shared_ptr<TaskIndependentEvaluator>>(
            "nested",
            "the evaluator that should be computed under modified costs");
        add_option<OperatorCost>(
            "cost_type",
            "Operator cost adjustment type. No matter what this setting is, "
            "axioms will always be considered as actions of cost 0 by the "
            "evaluators that treat axioms as actions.",
            "normal");

        document_language_support(
            "action costs", "supported if the nested evaluator does");
        document_language_support(
            "conditional effects", "supported if the nested evaluator does");
        document_language_support(
            "axioms", "supported if the nested evaluator does");

        document_property("admissible", "if the nested evaluator is");
        document_property("consistent", "if the nested evaluator is");
        document_property("safe", "if the nested evaluator is");
        document_property(
            "preferred operators", "if the nested evaluator supports them");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return make_shared<TaskIndependentCostAdaptedEvaluator>(
            opts.get<shared_ptr<TaskIndependentEvaluator>>("nested"),
            opts.get<OperatorCost>("cost_type"));
    }
};

static plugins::FeaturePlugin<CostAdaptedEvaluatorFeature> _plugin;
}
