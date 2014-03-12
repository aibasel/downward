#include "int_packer.h"

#include <cassert>
using namespace std;

static void set_bits(Bin &mask, unsigned int from, unsigned int to) {
    // Set all bits in the range [from, to) to 1.
    assert(from <= to);
    int length = to - from;
    assert(length < 32 && "1U << 32 has undefined behaviour");
    mask |= ((Bin(1) << length) - 1) << from;
}

static int get_needed_bits(int num_values) {
    unsigned int num_bits = 0;
    while (num_values > 1U << num_bits)
        ++num_bits;
    return num_bits;
}

static int get_max_fitting_bits(
    const vector<vector<int> > &bits_to_vars, int available_bits) {
    for (size_t bits = available_bits; bits != 0; --bits) {
        if (!bits_to_vars[bits].empty()) {
            return bits;
        }
    }
    return 0;
}


IntPacker::IntPacker(const vector<int> &ranges)
    : var_infos(),
      num_bins(0) {
    pack_bins(ranges);
}

IntPacker::~IntPacker() {
}

int IntPacker::get(const Bin *buffer, int var) const {
    const VariableInfo &var_info = var_infos[var];
    return (buffer[var_info.bin_index] & var_info.read_mask) >> var_info.shift;
}

void IntPacker::set(Bin *buffer, int var, int value) const {
    const VariableInfo &var_info = var_infos[var];
    assert(value < var_info.range);
    Bin before = buffer[var_info.bin_index];
    buffer[var_info.bin_index] = (before & var_info.clear_mask) |
                                 (value << var_info.shift);
}

void IntPacker::pack_bins(const vector<int> &ranges) {
    assert(var_infos.empty());
    int num_vars = ranges.size();
    var_infos.resize(num_vars);

    int bits_per_bin = sizeof(Bin) * 8;
    vector<vector<int> > bits_to_vars(bits_per_bin + 1);
    for (size_t var = 0; var < num_vars; ++var) {
        int bits = get_needed_bits(ranges[var]);
        assert(bits < bits_per_bin);
        bits_to_vars[bits].push_back(var);
    }
    // Greedy strategy: always add the largest fitting variable.
    int num_packed_vars = 0;
    num_bins = 1;
    int remaining_bits = bits_per_bin;
    while (num_packed_vars < num_vars) {
        int bits = get_max_fitting_bits(bits_to_vars, remaining_bits);
        if (bits == 0) {
            // We cannot pack another variable into the current bin,
            // so we add an additional bin.
            ++num_bins;
            remaining_bits = bits_per_bin;
            continue;
        }
        // We can pack another variable of size bits into the current bin.
        vector<int> &best_fit_vars = bits_to_vars[bits];
        assert(!best_fit_vars.empty());
        int var = best_fit_vars.back();
        best_fit_vars.pop_back();
        VariableInfo &var_info = var_infos[var];
        var_info.range = ranges[var];
        var_info.shift = bits_per_bin - remaining_bits;
        var_info.bin_index = num_bins - 1;
        var_info.read_mask = 0;
        set_bits(var_info.read_mask, var_info.shift, var_info.shift + bits);
        var_info.clear_mask = ~var_info.read_mask;
        remaining_bits -= bits;
        ++num_packed_vars;
    }
}
