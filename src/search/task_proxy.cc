#include "task_proxy.h"

#include "axioms.h"
#include "state_registry.h"

#include "task_utils/causal_graph.h"

#include <iostream>

using namespace std;

State::State(const AbstractTask &task, const StateRegistry *registry,
             const StateID &id, const PackedStateBin *buffer, vector<int> &&values)
    : task(&task), registry(registry), id(id), buffer(buffer), values(nullptr),
      state_packer(nullptr) {
    /*
      Either this is a registered state and all three of {registry, id, buffer}
      have valid values, or it is an unregistered state and none of them do.
    */
    assert((registry && id != StateID::no_state && buffer) ||
           (!registry && id == StateID::no_state && !buffer));

    if (!values.empty()) {
        this->values = make_shared<vector<int>>(move(values));
    }

    if (registry) {
        state_packer = &registry->get_state_packer();
        num_variables = registry->get_num_variables();
    } else {
        // If the state has no packed data, it has to have unpacked data.
        assert(this->values);
        num_variables = this->values->size();
    }
    assert(num_variables == this->task->get_num_variables());
}

State State::get_successor(const OperatorProxy &op) const {
    assert(!op.is_axiom());
    //assert(is_applicable(op, state));

    unpack();
    vector<int> new_values = get_values();

    for (EffectProxy effect : op.get_effects()) {
        if (does_fire(effect, *this)) {
            FactPair effect_fact = effect.get_fact().get_pair();
            new_values[effect_fact.var] = effect_fact.value;
        }
    }

    if (task->get_num_axioms() > 0) {
        AxiomEvaluator &axiom_evaluator = g_axiom_evaluators[TaskProxy(*task)];
        axiom_evaluator.evaluate(new_values);
    }
    return State(*task, nullptr, StateID::no_state, nullptr, move(new_values));
}

const causal_graph::CausalGraph &TaskProxy::get_causal_graph() const {
    return causal_graph::get_causal_graph(task);
}
