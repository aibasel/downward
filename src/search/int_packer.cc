#include "int_packer.h"

#include <cassert>
using namespace std;


class IntPacker::VariableInfo {
public:
    int range;
    int bin_index;
    int shift;
    Bin read_mask;
    Bin clear_mask;
};


static IntPacker::Bin get_bit_mask(int from, int to) {
    // Return mask with all bits in the range [from, to) set to 1.
    assert(from >= 0 && to >= from);
    int length = to - from;
    assert(length < 32); // 1U << 32 has undefined behaviour on 32-bit platforms
    return ((IntPacker::Bin(1) << length) - 1) << from;
}

static int get_needed_bitsize(int range) {
    int num_bits = 0;
    while ((1U << num_bits) < range)
        ++num_bits;
    return num_bits;
}

static int get_max_fitting_bits(
    const vector<vector<int> > &bits_to_vars, int available_bits) {
    for (int bits = available_bits; bits != 0; --bits) {
        if (!bits_to_vars[bits].empty()) {
            return bits;
        }
    }
    return 0;
}


IntPacker::IntPacker(const vector<int> &ranges)
    : num_bins(0) {
    pack_bins(ranges);
}

IntPacker::~IntPacker() {
}

int IntPacker::get(const Bin *buffer, int var) const {
    const VariableInfo &var_info = var_infos[var];
    Bin bin = buffer[var_info.bin_index];
    return (bin & var_info.read_mask) >> var_info.shift;
}

void IntPacker::set(Bin *buffer, int var, int value) const {
    const VariableInfo &var_info = var_infos[var];
    assert(value >= 0 && value < var_info.range);
    Bin &bin = buffer[var_info.bin_index];
    bin = (bin & var_info.clear_mask) | (value << var_info.shift);
}

void IntPacker::pack_bins(const vector<int> &ranges) {
    assert(var_infos.empty());
    int num_vars = ranges.size();
    var_infos.resize(num_vars);

    int bits_per_bin = sizeof(Bin) * 8;
    vector<vector<int> > bits_to_vars(bits_per_bin + 1);
    for (size_t var = 0; var < num_vars; ++var) {
        int bits = get_needed_bitsize(ranges[var]);
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
        int shift = bits_per_bin - remaining_bits;
        var_info.range = ranges[var];
        var_info.shift = shift;
        var_info.bin_index = num_bins - 1;
        var_info.read_mask = get_bit_mask(shift, shift + bits);
        var_info.clear_mask = ~var_info.read_mask;
        remaining_bits -= bits;
        ++num_packed_vars;
    }
}
