#include "state.h"

#include "globals.h"
#include "utilities.h"
#include "state_registry.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;

State::State(state_var_t *buffer, StateID id_)
    : vars(buffer),
      id(id_) {
    assert(vars);
    assert(id.is_valid());
}

State::~State() {
}

void State::dump_pddl() const {
    for (int i = 0; i < g_variable_domain.size(); i++) {
        const string &fact_name = g_fact_names[i][vars[i]];
        if (fact_name != "<none of those>")
            cout << fact_name << endl;
    }
}

void State::dump_fdr() const {
    // We cast the values to int since we'd get bad output otherwise
    // if state_var_t == char.
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        cout << "  #" << i << " [" << g_variable_name[i] << "] -> "
             << static_cast<int>(vars[i]) << endl;
}

bool State::operator==(const State &other) const {
    // TODO this will not work if there is more than one state registry
    if (id.is_valid()) {
        return id.value == other.id.value;
    } else if (vars == other.vars) {
        // borrowed from same buffer
        return true;
    } else {
        int size = g_variable_domain.size();
        return ::equal(vars, vars + size, other.vars);
    }
}

bool State::operator<(const State &other) const {
    int size = g_variable_domain.size();
    return ::lexicographical_compare(vars, vars + size,
                                     other.vars, other.vars + size);
}

size_t State::hash() const {
    return ::hash_number_sequence(vars, g_variable_domain.size());
}
