#include "concrete_state.h"

#include "../global_operator.h"
#include "../global_state.h"
#include "../state_registry.h"

using namespace std;

namespace cegar {

ConcreteState::ConcreteState(const GlobalState &state)
    : state(&state) {

}

ConcreteState::~ConcreteState() {
}

int ConcreteState::operator[](size_t index) const {
    return (*state)[index];
}

ConcreteState ConcreteState::apply(const GlobalOperator &op) const {
    return ConcreteState(const_cast<StateRegistry *>(state->registry)->get_successor_state(*state, op));
}

StateID ConcreteState::get_id() const {
    return state->get_id();
}

bool is_applicable(const GlobalCondition &condition, const ConcreteState &state) {
    return state[condition.var] == condition.val;
}

bool is_applicable(const GlobalOperator &op, const ConcreteState &state) {
    for (GlobalCondition precondition : op.get_preconditions()) {
        if (!is_applicable(precondition, state))
            return false;
    }
    return true;
}

bool is_goal_state(vector<pair<int, int>> goals, const ConcreteState &state) {
    for (auto goal : goals) {
        if (state[goal.first] != goal.second) {
            return false;
        }
    }
    return true;
}
}

