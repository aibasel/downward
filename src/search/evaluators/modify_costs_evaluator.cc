#include "modify_costs_evaluator.h"

#include "simple_task_transforming_evaluator.h"

#include "../evaluation_context.h"

#include "../plugins/plugin.h"
#include "../tasks/cost_adapted_task.h"

#include <utility>

using namespace std;

namespace cost_adapted_evaluator {
shared_ptr<Evaluator>
TaskIndependentModifyCostsEvaluator::create_task_specific_component(
    const shared_ptr<AbstractTask> &task) const {
    if (cost_type == OperatorCost::NORMAL) {
        return nested->bind_task(task);
    } else {
        shared_ptr<AbstractTask> cost_adapted_task =
            make_shared<tasks::CostAdaptedTask>(task, cost_type);
        shared_ptr<Evaluator> eval = nested->bind_task(cost_adapted_task);
        return make_shared<simple_task_transforming_evaluator::
                               SimpleTaskTransformingEvaluator>(
            task, eval, description, verbosity);
    }
}

TaskIndependentModifyCostsEvaluator::TaskIndependentModifyCostsEvaluator(
    shared_ptr<TaskIndependentEvaluator> nested, OperatorCost cost_type,
    const string &description, utils::Verbosity verbosity)
    : nested(move(nested)),
      cost_type(cost_type),
      description(description),
      verbosity(verbosity) {
}

class ModifyCostEvaluatorFeature
    : public plugins::TypedFeature<TaskIndependentEvaluator> {
public:
    ModifyCostEvaluatorFeature() : TypedFeature("eval_modify_costs") {
        document_title("Cost-modifying evaluator");
        document_subcategory("evaluators_basic");
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
        add_evaluator_options_to_feature(*this, "eval_modify_costs");

        document_language_support(
            "action costs", "supported if the nested evaluator supports them; "
                            "otherwise not supported");
        document_language_support(
            "conditional effects",
            "supported if the nested evaluator supports them; otherwise not "
            "supported");
        document_language_support(
            "axioms",
            "supported if the nested evaluator supports them; otherwise not "
            "supported");

        document_property(
            "admissible",
            "yes, if the nested evaluator is admissible and the cost "
            "modification does not increase the costs");
        document_property(
            "consistent",
            "yes, if the nested evaluator is consistent and the cost "
            "modification does not increase the costs");
        document_property("safe", "yes, if the nested evaluator is safe");
        document_property(
            "preferred operators",
            "yes, if the nested evaluator identifies preferred operators");
    }

    virtual shared_ptr<TaskIndependentEvaluator> create_component(
        const plugins::Options &opts) const override {
        return components::make_shared_from_arg_tuples<
            TaskIndependentModifyCostsEvaluator>(
            opts.get<shared_ptr<TaskIndependentEvaluator>>("nested"),
            opts.get<OperatorCost>("cost_type"),
            get_evaluator_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<ModifyCostEvaluatorFeature> _plugin;
}
