#include "task_proxy.h"

#include "axioms.h"
#include "state_registry.h"

#include "task_utils/causal_graph.h"

#include <iostream>

using namespace std;

StateData::StateData(
    const StateRegistry *registry, const StateID &id,
    const PackedStateBin *buffer, vector<int> &&values)
    : registry(registry), id(id), buffer(buffer), values(move(values)) {
    /*
      Either this is a registered state and all three of {registry, id, buffer}
      have valid values, or it is an unregistered state and none of them do.
    */
    assert((registry && id != StateID::no_state && buffer) ||
           (!registry && id == StateID::no_state && !buffer));
    /*
      The state has to have data, either packed, or unpacked, or both.
    */
    assert(buffer || !this->values.empty());
}

State::State(const AbstractTask &task, const StateRegistry *registry,
             const StateID &id, const PackedStateBin *buffer, vector<int> &&values)
    : task(&task), data(make_shared<StateData>(registry, id, buffer, move(values))) {
    assert(static_cast<int>(size()) == this->task->get_num_variables());
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

void StateData::unpack() const {
    if (!buffer) {
        cerr << "Tried to unpack an unregistered state. These states are not "
                "packed, so this is likely an error." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    assert(registry);
    const int_packer::IntPacker &state_packer = registry->get_state_packer();
    if (values.empty()) {
        int num_variables = size();
        values.reserve(num_variables);
        for (int var = 0; var < num_variables; ++var) {
            values.push_back(state_packer.get(buffer, var));
        }
    }
}

std::size_t StateData::size() const {
    if (registry) {
        return registry->get_num_variables();
    } else {
        assert(!values.empty());
        return values.size();
    }
}

int StateData::operator[](std::size_t var_id) const {
    assert(var_id < size());
    if (values.empty()) {
        assert(buffer);
        assert(registry);
        const int_packer::IntPacker &state_packer = registry->get_state_packer();
        return state_packer.get(buffer, var_id);
    } else {
        return values[var_id];
    }
}
