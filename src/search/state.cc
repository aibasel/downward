#include "state.h"

#include "globals.h"
#include "utilities.h"
#include "state_registry.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;

void set_bits(PackedStateEntry &mask, unsigned int from, unsigned int to) {
    assert (from <= to);
    mask |= (PackedStateEntry(1) << to) - (PackedStateEntry(1) << from);
}

int get_needed_bits(int num_values) {
    unsigned int num_bits = 0;
    while (num_values > 1U << num_bits)
        ++num_bits;
    return num_bits;
}

int get_max_fitting_bits(const vector<vector<int> > &vars_by_needed_bits,
                                 int available_bits) {
    for (size_t bits = available_bits; bits != 0; --bits) {
        if (!vars_by_needed_bits[bits].empty()) {
            return bits;
        }
    }
    return 0;
}


State::State(const PackedStateEntry *buffer_, const StateRegistry &registry_,
             StateID id_)
    : buffer(buffer_),
      registry(&registry_),
      id(id_) {
    assert(buffer);
    assert(id != StateID::no_state);
}

State::~State() {
}

int State::operator[](int index) const {
    return get(buffer, index);
}

int State::get(const PackedStateEntry *buffer, int var) {
    const PackedVariable &packed_var = packed_variables[var];
    return (buffer[packed_var.index] & packed_var.read_mask) >> packed_var.shift;
}

void State::set(PackedStateEntry *buffer, int var, int value) {
    const PackedVariable &packed_var = packed_variables[var];
    PackedStateEntry before = buffer[packed_var.index];
    buffer[packed_var.index] = (before & packed_var.clear_mask) | (value << packed_var.shift);
}

void State::pack_state() {
    assert(packed_variables.empty());
    int num_variables = g_variable_domain.size();
    packed_variables.resize(num_variables);

    int bits_per_entry = sizeof(PackedStateEntry) * 8;
    vector<vector<int> > vars_by_needed_bits(bits_per_entry + 1);
    for (size_t var = 0; var < num_variables; ++var) {
        int bits = get_needed_bits(g_variable_domain[var]);
        assert(bits < bits_per_entry);
        vars_by_needed_bits[bits].push_back(var);
    }
    // Greedy strategy: always add the largest fitting variable.
    int num_packed_variables = 0;
    packed_size = 1;
    int remaining_bits = bits_per_entry;
    while (num_packed_variables < num_variables) {
        int bits = get_max_fitting_bits(vars_by_needed_bits, remaining_bits);
        if (bits == 0) {
            // We cannot pack another variable into the current word so we add
            // an additional word to the state.
            ++packed_size;
            remaining_bits = bits_per_entry;
            continue;
        }
        // We can pack another value of size max_fitting_bits into the current word.
        vector<int> &vars = vars_by_needed_bits[bits];
        assert(!vars.empty());
        int var = vars.back();
        vars.pop_back();
        PackedVariable &packed_var = packed_variables[var];
        packed_var.shift = bits_per_entry - remaining_bits;
        packed_var.index = packed_size - 1;
        packed_var.read_mask = 0;
        set_bits(packed_var.read_mask, packed_var.shift, packed_var.shift + bits);
        packed_var.clear_mask = ~packed_var.read_mask;
        remaining_bits -= bits;
        ++num_packed_variables;
    }
    cout << "Variables: " << g_variable_domain.size() << endl;
    cout << "Bytes per state: " << packed_size * sizeof(PackedStateEntry) << endl;
}

void State::dump_pddl() const {
    for (int i = 0; i < g_variable_domain.size(); i++) {
        const string &fact_name = g_fact_names[i][(*this)[i]];
        if (fact_name != "<none of those>")
            cout << fact_name << endl;
    }
}

void State::dump_fdr() const {
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        cout << "  #" << i << " [" << g_variable_name[i] << "] -> "
             << (*this)[i] << endl;
}

std::vector<State::PackedVariable> State::packed_variables;
int State::packed_size = 0;
