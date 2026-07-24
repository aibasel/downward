#include "modify_costs_evaluator.h"

#include "../evaluation_context.h"

#include "../plugins/plugin.h"
#include "../tasks/cost_adapted_task.h"

#include <utility>

using namespace std;

namespace cost_adapted_evaluator {
ModifyCostsEvaluator::ModifyCostsEvaluator(
    const shared_ptr<AbstractTask> &task, const shared_ptr<Evaluator> &nested,
    const string &description, utils::Verbosity verbosity)
    : Evaluator(task, false, false, false, description, verbosity),
      nested(nested) {
}

State ModifyCostsEvaluator::convert_state(const State &state) const {
    const StateRegistry *existing_state_registry = state.get_registry();
    assert(existing_state_registry);
    if (!state_registry) {
        /*
          issue1227:
            * avoid const cast?
              -> currently needed because we only get the registry from states
                 passed to this evaluator instead of knowing the registry in the
                 constructor already. We only get a const registry pointer from
                 the state but need it to be mutable in the delegating registry.
            * avoid mutable state_registry?
              -> currently this is used for the delayed initialization which can
                 happen inside const methods.
            * avoid delayed initialization alltogether
              Both of the above problems would go away if we could create the
              registry in the constructor.
        */
        state_registry = make_unique<DelegatingStateRegistry>(
            task, const_cast<StateRegistry &>(*existing_state_registry));
    } else {
        state_registry->assert_nested_is(*existing_state_registry);
    }
    return state_registry->convert_state(state);
}

bool ModifyCostsEvaluator::dead_ends_are_reliable() const {
    return nested->dead_ends_are_reliable();
}

void ModifyCostsEvaluator::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    nested->get_path_dependent_evaluators(evals);
}

void ModifyCostsEvaluator::notify_initial_state(const State &initial_state) {
    nested->notify_initial_state(convert_state(initial_state));
}

void ModifyCostsEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    nested->notify_state_transition(
        convert_state(parent_state), op_id, convert_state(state));
}

EvaluationResult ModifyCostsEvaluator::compute_result(
    EvaluationContext &eval_context) {
    State translated_state = convert_state(eval_context.get_state());
    EvaluationContext translated_context(eval_context, translated_state);
    /*
      Note that we do not need to update the cache inside eval_context.
      the call below can only set and access evaluators that use the transformed
      task and users of eval_context cannot know such evaluators.
    */
    return nested->compute_result(translated_context);
}

bool ModifyCostsEvaluator::does_cache_estimates() const {
    return nested->does_cache_estimates();
}

bool ModifyCostsEvaluator::is_estimate_cached(const State &state) const {
    return nested->is_estimate_cached(convert_state(state));
}

int ModifyCostsEvaluator::get_cached_estimate(const State &state) const {
    return nested->get_cached_estimate(convert_state(state));
}

shared_ptr<Evaluator>
TaskIndependentModifyCostsEvaluator::create_task_specific_component(
    const shared_ptr<AbstractTask> &task) const {
    if (cost_type == OperatorCost::NORMAL) {
        return nested->bind_task(task);
    } else {
        shared_ptr<AbstractTask> cost_adapted_task =
            make_shared<tasks::CostAdaptedTask>(task, cost_type);
        shared_ptr<Evaluator> eval = nested->bind_task(cost_adapted_task);
        return make_shared<ModifyCostsEvaluator>(
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
