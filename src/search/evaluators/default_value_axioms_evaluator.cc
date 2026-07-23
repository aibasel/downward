#include "default_value_axioms_evaluator.h"

#include "../evaluation_context.h"

#include "../task_utils/task_properties.h"

#include <utility>

using namespace std;

namespace default_value_axioms_evaluator {
DefaultValueAxiomsEvaluator::DefaultValueAxiomsEvaluator(
    const shared_ptr<AbstractTask> &task, const shared_ptr<Evaluator> &nested,
    bool use_for_reporting_minima, bool use_for_boosting,
    bool use_for_counting_evaluations, const string &description,
    utils::Verbosity verbosity)
    : Evaluator(
          task, use_for_reporting_minima, use_for_boosting,
          use_for_counting_evaluations, description, verbosity),
      nested(nested) {
}

State DefaultValueAxiomsEvaluator::repackage_state(const State &state) const {
    const StateRegistry *existing_state_registry = state.get_registry();
    if (!state_registry) {
        // issue1208: avoid const cast?
        state_registry = make_unique<DelegatingStateRegistry>(
            task, const_cast<StateRegistry &>(*existing_state_registry));
    } else {
        // issue1208: verify that state_registry.nested ==
        // existing_state_registry? or use a map from registries to delegating
        // registries?
    }
    return state_registry->repackage_state(state);
}

bool DefaultValueAxiomsEvaluator::dead_ends_are_reliable() const {
    return nested->dead_ends_are_reliable();
}

void DefaultValueAxiomsEvaluator::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    nested->get_path_dependent_evaluators(evals);
}

void DefaultValueAxiomsEvaluator::notify_initial_state(
    const State &initial_state) {
    nested->notify_initial_state(repackage_state(initial_state));
}

void DefaultValueAxiomsEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    nested->notify_state_transition(
        repackage_state(parent_state), op_id, repackage_state(state));
}

EvaluationResult DefaultValueAxiomsEvaluator::compute_result(
    EvaluationContext &eval_context) {
    State translated_state = repackage_state(eval_context.get_state());
    EvaluationContext translated_context(eval_context, translated_state);
    EvaluationResult result = nested->compute_result(translated_context);
    return result;
    // TODO do we need something like this?
    //return eval_context.add_to_cache(nested.get(), move(result));
}

bool DefaultValueAxiomsEvaluator::does_cache_estimates() const {
    return nested->does_cache_estimates();
}

bool DefaultValueAxiomsEvaluator::is_estimate_cached(const State &state) const {
    return nested->is_estimate_cached(repackage_state(state));
}

int DefaultValueAxiomsEvaluator::get_cached_estimate(const State &state) const {
    return nested->get_cached_estimate(repackage_state(state));
}

shared_ptr<Evaluator>
TaskIndependentDefaultValueAxiomsEvaluator::create_task_specific_component(
    const shared_ptr<AbstractTask> &task) const {
    TaskProxy proxy(*task);
    if (task_properties::has_axioms(proxy)) {
        shared_ptr<AbstractTask> axioms_task =
            make_shared<tasks::DefaultValueAxiomsTask>(task, axioms);
        shared_ptr<Evaluator> eval = nested->bind_task(axioms_task);
        return make_shared<DefaultValueAxiomsEvaluator>(
            task, eval, use_for_reporting_minima, use_for_boosting,
            use_for_counting_evaluations, description, verbosity);
    } else {
        /*
          If the task has no axioms, there is no need to wrap the evaluator
          and we can bind the unmodified task to the nested evaluator directly.
        */
        return nested->bind_task(task);
    }
}

TaskIndependentDefaultValueAxiomsEvaluator::
    TaskIndependentDefaultValueAxiomsEvaluator(
        shared_ptr<TaskIndependentEvaluator> nested,
        tasks::AxiomHandlingType axioms, bool use_for_reporting_minima,
        bool use_for_boosting, bool use_for_counting_evaluations,
        const string &description, utils::Verbosity verbosity)
    : nested(move(nested)),
      axioms(axioms),
      use_for_reporting_minima(use_for_reporting_minima),
      use_for_boosting(use_for_boosting),
      use_for_counting_evaluations(use_for_counting_evaluations),
      description(description),
      verbosity(verbosity) {
}

shared_ptr<TaskIndependentEvaluator> wrap_in_default_axiom_evaluator(
    const shared_ptr<TaskIndependentEvaluator> &eval,
    const plugins::Options &opts) {
    return components::make_shared_from_arg_tuples<
        default_value_axioms_evaluator::
            TaskIndependentDefaultValueAxiomsEvaluator>(
        eval, tasks::get_axioms_arguments_from_options(opts), true, true, true,
        get_evaluator_arguments_from_options(opts));
}

}
