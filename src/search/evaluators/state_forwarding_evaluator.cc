#include "state_forwarding_evaluator.h"

#include "../evaluator_call.h"

using namespace std;

namespace state_forwarding_evaluator {
StateForwardingEvaluator::StateForwardingEvaluator(
    const shared_ptr<AbstractTask> &task,
    const shared_ptr<AbstractTask> &transformed_task,
    const shared_ptr<Evaluator> &nested, const string &description,
    utils::Verbosity verbosity)
    : Evaluator(task, false, false, false, description, verbosity),
      transformed_task(transformed_task),
      nested(nested) {
    set<Evaluator *> evals;
    nested->get_path_dependent_evaluators(evals);
    path_dependent_evaluators.assign(evals.begin(), evals.end());
}

State StateForwardingEvaluator::convert_state(const State &state) const {
    /*
      issue1227:
        * avoid dispatching over incoming registries
          -> currently using a heuristic in multiple contexts can lead to states
             coming from different registries. For example
             let(hcg,eval_modify_costs(cg(),cost_type=plusone),
                 iterated([eager_greedy([hcg]), eager_greedy([hcg])]))
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
    const StateRegistry *existing_state_registry = state.get_registry();
    DelegatingStateRegistry *delegating_registry;
    assert(existing_state_registry);
    if (existing_state_registry == last_state_registry_key) {
        delegating_registry = last_state_registry;
    } else {
        if (!state_registries.contains(existing_state_registry)) {
            state_registries.emplace(
                existing_state_registry,
                make_unique<DelegatingStateRegistry>(
                    transformed_task,
                    const_cast<StateRegistry &>(*existing_state_registry)));
        }
        delegating_registry =
            state_registries.at(existing_state_registry).get();
        last_state_registry_key = existing_state_registry;
        last_state_registry = delegating_registry;
    }
    return delegating_registry->convert_state(state);
}

bool StateForwardingEvaluator::is_safe() const {
    return nested->is_safe();
}

void StateForwardingEvaluator::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    if (!path_dependent_evaluators.empty()) {
        /*
          Note that we do *not* add path_dependent_evaluators to this list
          because they use a differnent task. Letting some code from the outside
          the scope of this task transformation notify them directly would
          bypass the task transformation and thus notify the nested evaluators
          with states from incorrect tasks. Instead, this evaluator adds itself
          and then forwards any events with the transformed task.
        */
        evals.insert(this);
    }
}

void StateForwardingEvaluator::notify_initial_state(
    const State &initial_state) {
    if (!path_dependent_evaluators.empty()) {
        State converted_initial_state = convert_state(initial_state);
        for (Evaluator *eval : path_dependent_evaluators) {
            eval->notify_initial_state(converted_initial_state);
        }
    }
}

void StateForwardingEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    if (!path_dependent_evaluators.empty()) {
        State converted_parent_state = convert_state(parent_state);
        State converted_state = convert_state(state);
        for (Evaluator *eval : path_dependent_evaluators) {
            eval->notify_state_transition(
                converted_parent_state, op_id, converted_state);
        }
    }
}

EvaluationResult StateForwardingEvaluator::compute_result(
    EvaluatorCall &call) {
    State translated_state = convert_state(call.get_state());
    EvaluatorCall subcall = call.get_subcall(translated_state);
    /*
      TODO issue1208: this copies the result. Does this make sense?
    */
    return subcall[nested.get()];
}

bool StateForwardingEvaluator::does_cache_estimates() const {
    return nested->does_cache_estimates();
}

bool StateForwardingEvaluator::is_estimate_cached(const State &state) const {
    return nested->is_estimate_cached(convert_state(state));
}

int StateForwardingEvaluator::get_cached_estimate(const State &state) const {
    return nested->get_cached_estimate(convert_state(state));
}
}
