#include "global_state.h"

#include "globals.h"
#include "state_registry.h"

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

void GlobalState::dump_pddl() const {
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        const string &fact_name = g_fact_names[i][(*this)[i]];
        if (fact_name != "<none of those>")
            cout << fact_name << endl;
    }
}

void GlobalState::dump_fdr() const {
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        cout << "  #" << i << " [" << g_variable_name[i] << "] -> "
             << (*this)[i] << endl;
}
