#include "concrete_state.h"

#include "../global_operator.h"
#include "../global_state.h"

using namespace std;

namespace cegar {

ConcreteState::ConcreteState(const GlobalState &state)
    : state(state) {

}

ConcreteState::~ConcreteState() {
}

int ConcreteState::operator[](size_t index) const {
    return state[index];
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
}

