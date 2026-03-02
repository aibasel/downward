#include "modify_costs_evaluator.h"

#include "../evaluation_context.h"

#include "../plugins/plugin.h"
#include "../tasks/cost_adapted_task.h"

#include <utility>

using namespace std;

namespace cost_adapted_evaluator {
bool ModifyCostsEvaluator::dead_ends_are_reliable() const {
    return nested->dead_ends_are_reliable();
}

void ModifyCostsEvaluator::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    nested->get_path_dependent_evaluators(evals);
}

void ModifyCostsEvaluator::notify_initial_state(const State &initial_state) {
    /*
      TODO issue1208: Once we remove the task transformation code from
      heuristics, we might have to transform the task here. While the state data
      doesn't change, the State class currently references the task it belongs
      to, and `nested` uses a different task.
    */
    nested->notify_initial_state(initial_state);
}

void ModifyCostsEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    // TODO issue1208: see above
    nested->notify_state_transition(parent_state, op_id, state);
}

EvaluationResult ModifyCostsEvaluator::compute_result(
    EvaluationContext &eval_context) {
    // TODO issue1208: see above (in particular, eval_context is specific to a
    // state)
    return nested->compute_result(eval_context);
}

bool ModifyCostsEvaluator::does_cache_estimates() const {
    return nested->does_cache_estimates();
}

bool ModifyCostsEvaluator::is_estimate_cached(const State &state) const {
    // TODO issue1208: see above
    return nested->is_estimate_cached(state);
}

int ModifyCostsEvaluator::get_cached_estimate(const State &state) const {
    // TODO issue1208: see above
    return nested->get_cached_estimate(state);
}

shared_ptr<Evaluator>
TaskIndependentModifyCostsEvaluator::create_task_specific_component(
    const shared_ptr<AbstractTask> &task, components::Cache &cache) const {
    shared_ptr<AbstractTask> cost_adapted_task =
        make_shared<tasks::CostAdaptedTask>(task, cost_type);
    return nested->bind_task(cost_adapted_task, cache);
}

TaskIndependentModifyCostsEvaluator::TaskIndependentModifyCostsEvaluator(
    shared_ptr<TaskIndependentEvaluator> nested, OperatorCost cost_type)
    : nested(move(nested)), cost_type(cost_type) {
}

class ModifyCostEvaluatorFeature
    : public plugins::TaskIndependentFeature<TaskIndependentEvaluator> {
public:
    ModifyCostEvaluatorFeature() : TaskIndependentFeature("eval_modify_costs") {
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
        return make_shared<TaskIndependentModifyCostsEvaluator>(
            opts.get<shared_ptr<TaskIndependentEvaluator>>("nested"),
            opts.get<OperatorCost>("cost_type"));
    }
};

static plugins::FeaturePlugin<ModifyCostEvaluatorFeature> _plugin;
}
