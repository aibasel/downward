#include "packed_state.h"

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

PackedStateProperties::PackedStateProperties(const vector<int> &variable_domain) {
    variables = new PackedVariable[variable_domain.size()];

    int available_bits = sizeof(PackedStateEntry) * 8;
    int max_needed_bits = 0;
    vector<vector<int> > vars_by_needed_bits(available_bits);
    for (size_t var_id = 0; var_id < variable_domain.size(); ++var_id) {
        int bits = bits_needed(variable_domain[var_id]);
        vars_by_needed_bits[bits].push_back(var_id);
        max_needed_bits = max(max_needed_bits, bits);
    }
    // Greedy strategy: always add the largest fitting variable.
    assert(max_needed_bits < available_bits);
    int remaining_bits = 0;
    int max_fitting_bits = 0;
    state_size = 0;
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
                ++state_size;
                remaining_bits = available_bits;
                max_fitting_bits = max_needed_bits;
            }
        } else {
            // We can pack another value of size max_fitting_bits into the current word.
            int var_id = vars_by_needed_bits[max_fitting_bits].back();
            vars_by_needed_bits[max_fitting_bits].pop_back();
            PackedVariable &var = variables[var_id];
            var.shift = available_bits - remaining_bits;
            var.index = state_size - 1;
            var.read_mask = 0;
            set_bits(var.read_mask, var.shift, var.shift + max_fitting_bits);
            var.clear_mask = ~var.read_mask;
            remaining_bits -= max_fitting_bits;
        }
    }
}

PackedStateProperties::~PackedStateProperties() {
    delete[] variables;
}
