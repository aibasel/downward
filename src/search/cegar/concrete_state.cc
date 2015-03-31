#include "concrete_state.h"

#include "../global_state.h"

using namespace std;

ConcreteState::ConcreteState(const GlobalState &state)
    : state(state) {

}

ConcreteState::~ConcreteState() {
}

int ConcreteState::operator[](size_t index) const {
    return state[index];
}

