#include "state.h"

#include "globals.h"
#include "utilities.h"
#include "state_registry.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;

void set_bits(PackedStateEntry &mask, unsigned int from, const unsigned int to) {
    for (; from < to; ++from) {
        mask |= (1 << from);
    }
}

int bits_needed(int domain) {
    unsigned int bits_needed = 0;
    domain--;
    do {
        domain >>= 1;
        ++bits_needed;
    } while (domain);
    return bits_needed;
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

int State::get(const PackedStateEntry *buffer, int index) {
    const PackedVariable &var = packed_variables[index];
    return (buffer[var.index] & var.read_mask) >> var.shift;
}

void State::set(PackedStateEntry *buffer, int index, int value) {
    const PackedVariable &var = packed_variables[index];
    PackedStateEntry before = buffer[var.index];
    buffer[var.index] = (before & var.clear_mask) | (value << var.shift);
}

void State::calculate_packed_size() {
    assert(g_packed_variables.empty());
    packed_variables.resize(g_variable_domain.size());

    int available_bits = sizeof(PackedStateEntry) * 8;
    int max_needed_bits = 0;
    vector<vector<int> > vars_by_needed_bits(available_bits);
    for (size_t var_id = 0; var_id < g_variable_domain.size(); ++var_id) {
        int bits = bits_needed(g_variable_domain[var_id]);
        vars_by_needed_bits[bits].push_back(var_id);
        max_needed_bits = max(max_needed_bits, bits);
    }
    // Greedy strategy: always add the largest fitting variable.
    assert(max_needed_bits < available_bits);
    int remaining_bits = 0;
    int max_fitting_bits = 0;
    packed_size = 0;
    while (max_needed_bits > 0) {
        while (max_fitting_bits > 0 &&
               (max_fitting_bits > remaining_bits ||
                vars_by_needed_bits[max_fitting_bits].empty())) {
            --max_fitting_bits;
        }

        if (max_fitting_bits == 0) {
            // We cannot pack another value into the current word.
            // Continue with the largest variable still available.
            while (max_needed_bits > 0 && vars_by_needed_bits[max_needed_bits].empty()) {
                --max_needed_bits;
            }
            if (max_needed_bits > 0) {
                // Start a new word.
                ++packed_size;
                remaining_bits = available_bits;
                max_fitting_bits = max_needed_bits;
            }
        } else {
            // We can pack another value of size max_fitting_bits into the current word.
            int var_id = vars_by_needed_bits[max_fitting_bits].back();
            vars_by_needed_bits[max_fitting_bits].pop_back();
            PackedVariable &var = packed_variables[var_id];
            var.shift = available_bits - remaining_bits;
            var.index = packed_size - 1;
            var.read_mask = 0;
            set_bits(var.read_mask, var.shift, var.shift + max_fitting_bits);
            var.clear_mask = ~var.read_mask;
            remaining_bits -= max_fitting_bits;
        }
    }
    cout << "Variables: " << g_variable_domain.size() << endl;
    cout << "PackedState size: "
         << packed_size * sizeof(PackedStateEntry) << endl;
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
