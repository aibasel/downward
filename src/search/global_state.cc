#include "global_state.h"

#include "state_registry.h"
#include "task_proxy.h"

#include "task_utils/task_properties.h"

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
    assert(var >= 0);
    assert(var < registry->get_num_variables());
    return registry->get_state_value(buffer, var);
}

State GlobalState::unpack() const {
    int num_variables = registry->get_num_variables();
    vector<int> values(num_variables);
    for (int var = 0; var < num_variables; ++var)
        values[var] = (*this)[var];
    TaskProxy task_proxy = registry->get_task_proxy();
    return task_proxy.create_state(move(values));
}

void GlobalState::dump_pddl() const {
    State state = unpack();
    task_properties::dump_pddl(state);
}

void GlobalState::dump_fdr() const {
    State state = unpack();
    task_properties::dump_fdr(state);
}
