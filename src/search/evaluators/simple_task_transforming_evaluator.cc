#include "simple_task_transforming_evaluator.h"

#include "../evaluation_context.h"

using namespace std;

namespace simple_task_transforming_evaluator {
SimpleTaskTransformingEvaluator::SimpleTaskTransformingEvaluator(
    const shared_ptr<AbstractTask> &task, const shared_ptr<Evaluator> &nested,
    const string &description, utils::Verbosity verbosity)
    : Evaluator(task, false, false, false, description, verbosity),
      nested(nested) {
}

State SimpleTaskTransformingEvaluator::convert_state(const State &state) const {
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

bool SimpleTaskTransformingEvaluator::dead_ends_are_reliable() const {
    return nested->dead_ends_are_reliable();
}

void SimpleTaskTransformingEvaluator::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    nested->get_path_dependent_evaluators(evals);
}

void SimpleTaskTransformingEvaluator::notify_initial_state(
    const State &initial_state) {
    nested->notify_initial_state(convert_state(initial_state));
}

void SimpleTaskTransformingEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    nested->notify_state_transition(
        convert_state(parent_state), op_id, convert_state(state));
}

EvaluationResult SimpleTaskTransformingEvaluator::compute_result(
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

bool SimpleTaskTransformingEvaluator::does_cache_estimates() const {
    return nested->does_cache_estimates();
}

bool SimpleTaskTransformingEvaluator::is_estimate_cached(
    const State &state) const {
    return nested->is_estimate_cached(convert_state(state));
}

int SimpleTaskTransformingEvaluator::get_cached_estimate(
    const State &state) const {
    return nested->get_cached_estimate(convert_state(state));
}
}
