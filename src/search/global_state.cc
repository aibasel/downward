#include "global_state.h"

#include "globals.h"
#include "state_registry.h"
#include "task_proxy.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;


GlobalState::GlobalState(
    const PackedStateBin *buffer,
    const StateRegistry &registry,
    StateID id,
    int num_variables)
    : buffer(buffer),
      registry(&registry),
      id(id),
      num_variables(num_variables) {
    assert(buffer);
    assert(id != StateID::no_state);
}

int GlobalState::operator[](int var) const {
    assert(var >= 0);
    assert(var < num_variables);
    return registry->get_state_value(buffer, var);
}

vector<int> GlobalState::get_values() const {
    vector<int> values(num_variables);
    for (int var = 0; var < num_variables; ++var)
        values[var] = (*this)[var];
    return values;
}

void dump_pddl(const GlobalState &global_state, const AbstractTask &task) {
    State state(task, global_state.get_values());
    state.dump_pddl();

}

void dump_fdr(const GlobalState &global_state, const AbstractTask &task) {
    State state(task, global_state.get_values());
    state.dump_fdr();

}
