#include "global_state.h"

#include "globals.h"
#include "state_registry.h"
#include "task_proxy.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;


GlobalState::GlobalState(
    const PackedStateBin *buffer, const StateRegistry &registry, StateID id)
    : buffer(buffer),
      registry(&registry),
      id(id) {
    assert(buffer);
    assert(id != StateID::no_state);
}

int GlobalState::operator[](int var) const {
    return registry->get_state_value(buffer, var);
}

vector<int> GlobalState::get_values() const {
    int num_vars = g_variable_domain.size();
    vector<int> values(num_vars);
    for (int var = 0; var < num_vars; ++var)
        values[var] = (*this)[var];
    return values;
}

void dump_pddl(const GlobalState &global_state, const AbstractTask &task) {
    vector<int> values = global_state.get_values();
    State state(task, move(values));
    state.dump_pddl();

}

void dump_fdr(const GlobalState &global_state, const AbstractTask &task) {
    vector<int> values = global_state.get_values();
    State state(task, move(values));
    state.dump_fdr();

}
