#include "concrete_state.h"

#include "../global_operator.h"
#include "../global_state.h"
#include "../state_registry.h"

using namespace std;

namespace cegar {

ConcreteState::ConcreteState(const GlobalState &state, shared_ptr<StateRegistry> registry)
    : state(state),
      registry(registry) {
}

ConcreteState::~ConcreteState() {
}

ConcreteState &ConcreteState::operator=(const ConcreteState &other) {
    state = other.state;
    registry = other.registry;
    return *this;
}

int ConcreteState::operator[](size_t index) const {
    return state[index];
}

ConcreteState ConcreteState::apply(const GlobalOperator &op) const {
    return ConcreteState(registry->get_successor_state(state, op), registry);
}

ConcreteState ConcreteState::apply(OperatorProxy op) const {
    return ConcreteState(registry->get_successor_state(state, op), registry);
}

StateID ConcreteState::get_id() const {
    return state.get_id();
}

bool is_applicable(const GlobalOperator &op, const ConcreteState &state) {
    for (GlobalCondition precondition : op.get_preconditions()) {
        if (state[precondition.var] != precondition.val)
            return false;
    }
    return true;
}

bool is_applicable(OperatorProxy op, const ConcreteState &state) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (state[precondition.get_variable().get_id()] != precondition.get_value())
            return false;
    }
    return true;
}

bool is_goal_state(TaskProxy task, const ConcreteState &state) {
    for (FactProxy goal : task.get_goals()) {
        if (state[goal.get_variable().get_id()] != goal.get_value()) {
            return false;
        }
    }
    return true;
}

ConcreteState get_initial_state(TaskProxy task_proxy) {
    vector<int> initial_state_data;
    initial_state_data.reserve(task_proxy.get_variables().size());
    for (FactProxy fact : task_proxy.get_initial_state())
        initial_state_data.push_back(fact.get_value());
    shared_ptr<StateRegistry> registry = get_state_registry(initial_state_data);
    return ConcreteState(registry->get_initial_state(), registry);
}
}

