#include "global_state.h"

#include "globals.h"
#include "state_registry.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;


GlobalState::GlobalState(const PackedStateBin *buffer_, const StateRegistry &registry_,
                         StateID id_)
    : buffer(buffer_),
      registry(&registry_),
      id(id_) {
    assert(buffer);
    assert(id != StateID::no_state);
}

GlobalState::~GlobalState() {
}

int GlobalState::operator[](size_t index) const {
    return g_state_packer->get(buffer, index);
}

vector<int> GlobalState::get_values() const {
    int num_vars = g_variable_domain.size();
    vector<int> values(num_vars);
    for (int var = 0; var < num_vars; ++var)
        values[var] = (*this)[var];
    return values;
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
