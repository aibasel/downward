#include "packed_state.h"

using namespace std;

void set_bits(PackedStateEntry &mask, unsigned int from, const unsigned int to) {
    for (; from < to; ++from) {
        mask |= (1 << from);
    }
}

PackedStateProperties::PackedStateProperties(const vector<int> &variable_domain) {
    variables = new PackedVariable[variable_domain.size()];

    // TODO use less than one PackedStateEntry per variable
    PackedStateEntry all_ones = 0;
    set_bits(all_ones, 0, sizeof(PackedStateEntry) * 8);
    for (size_t var_id = 0; var_id < variable_domain.size(); ++var_id) {
        PackedVariable &var = variables[var_id];
        var.shift = 0;
        var.index = var_id;
        var.read_mask = all_ones;
        var.clear_mask = ~var.read_mask;
    }
    state_size = variable_domain.size();
}

PackedStateProperties::~PackedStateProperties() {
    delete[] variables;
}
