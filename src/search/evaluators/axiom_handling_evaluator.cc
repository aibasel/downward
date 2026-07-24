#include "axiom_handling_evaluator.h"

#include "../evaluation_context.h"

#include "../task_utils/task_properties.h"

using namespace std;

namespace axiom_handling_evaluator {
AxiomHandlingEvaluator::AxiomHandlingEvaluator(
    const shared_ptr<AbstractTask> &task, const shared_ptr<Evaluator> &nested,
    bool use_for_reporting_minima, bool use_for_boosting,
    bool use_for_counting_evaluations, const string &description,
    utils::Verbosity verbosity)
    : Evaluator(
          task, use_for_reporting_minima, use_for_boosting,
          use_for_counting_evaluations, description, verbosity),
      nested(nested) {
}

State AxiomHandlingEvaluator::convert_state(const State &state) const {
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

bool AxiomHandlingEvaluator::dead_ends_are_reliable() const {
    return nested->dead_ends_are_reliable();
}

void AxiomHandlingEvaluator::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    nested->get_path_dependent_evaluators(evals);
}

void AxiomHandlingEvaluator::notify_initial_state(const State &initial_state) {
    nested->notify_initial_state(convert_state(initial_state));
}

void AxiomHandlingEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    nested->notify_state_transition(
        convert_state(parent_state), op_id, convert_state(state));
}

EvaluationResult AxiomHandlingEvaluator::compute_result(
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

bool AxiomHandlingEvaluator::does_cache_estimates() const {
    return nested->does_cache_estimates();
}

bool AxiomHandlingEvaluator::is_estimate_cached(const State &state) const {
    return nested->is_estimate_cached(convert_state(state));
}

int AxiomHandlingEvaluator::get_cached_estimate(const State &state) const {
    return nested->get_cached_estimate(convert_state(state));
}

shared_ptr<Evaluator>
TaskIndependentAxiomHandlingEvaluator::create_task_specific_component(
    const shared_ptr<AbstractTask> &task) const {
    TaskProxy proxy(*task);
    if (task_properties::has_axioms(proxy)) {
        shared_ptr<AbstractTask> axioms_task =
            make_shared<tasks::DefaultValueAxiomsTask>(task, axioms);
        shared_ptr<Evaluator> eval = nested->bind_task(axioms_task);
        return make_shared<AxiomHandlingEvaluator>(
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

TaskIndependentAxiomHandlingEvaluator::TaskIndependentAxiomHandlingEvaluator(
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

shared_ptr<TaskIndependentEvaluator> wrap_in_axiom_handling_evaluator(
    const shared_ptr<TaskIndependentEvaluator> &eval,
    const plugins::Options &opts) {
    return components::make_shared_from_arg_tuples<
        axiom_handling_evaluator::TaskIndependentAxiomHandlingEvaluator>(
        eval, tasks::get_axioms_arguments_from_options(opts), true, true, true,
        get_evaluator_arguments_from_options(opts));
}

}
