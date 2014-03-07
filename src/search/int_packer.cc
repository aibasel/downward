#include "int_packer.h"

#include "utilities.h"

#include <cassert>
#include <iostream>
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

int get_max_fitting_bits(const vector<vector<int> > &ints_by_needed_bits,
                                 int available_bits) {
    for (size_t bits = available_bits; bits != 0; --bits) {
        if (!ints_by_needed_bits[bits].empty()) {
            return bits;
        }
    }
    return 0;
}


IntPacker::IntPacker(const vector<int> &ranges)
    : packed_ints(),
      packed_size(0) {
    pack_bins(ranges);
}

IntPacker::~IntPacker() {
}

int IntPacker::get(const PackedStateEntry *buffer, int pos) const {
    const PackedInt &packed_int = packed_ints[pos];
    return (buffer[packed_int.index] & packed_int.read_mask) >> packed_int.shift;
}

void IntPacker::set(PackedStateEntry *buffer, int pos, int value) const {
    const PackedInt &packed_int = packed_ints[pos];
    PackedStateEntry before = buffer[packed_int.index];
    buffer[packed_int.index] = (before & packed_int.clear_mask) | (value << packed_int.shift);
}

void IntPacker::pack_bins(const vector<int> &ranges) {
    assert(packed_ints.empty());
    int num_ints = ranges.size();
    packed_ints.resize(num_ints);

    int bits_per_entry = sizeof(PackedStateEntry) * 8;
    vector<vector<int> > ints_by_needed_bits(bits_per_entry + 1);
    for (size_t pos = 0; pos < num_ints; ++pos) {
        int bits = get_needed_bits(ranges[pos]);
        assert(bits < bits_per_entry);
        ints_by_needed_bits[bits].push_back(pos);
    }
    // Greedy strategy: always add the largest fitting int.
    int num_packed_ints = 0;
    packed_size = 1;
    int remaining_bits = bits_per_entry;
    while (num_packed_ints < num_ints) {
        int bits = get_max_fitting_bits(ints_by_needed_bits, remaining_bits);
        if (bits == 0) {
            // We cannot pack another int into the current word so we add
            // an additional word.
            ++packed_size;
            remaining_bits = bits_per_entry;
            continue;
        }
        // We can pack another value of size max_fitting_bits into the current word.
        vector<int> &ints = ints_by_needed_bits[bits];
        assert(!ints.empty());
        int pos = ints.back();
        ints.pop_back();
        PackedInt &packed_int = packed_ints[pos];
        packed_int.shift = bits_per_entry - remaining_bits;
        packed_int.index = packed_size - 1;
        packed_int.read_mask = 0;
        set_bits(packed_int.read_mask, packed_int.shift, packed_int.shift + bits);
        packed_int.clear_mask = ~packed_int.read_mask;
        remaining_bits -= bits;
        ++num_packed_ints;
    }
    cout << "Ints: " << ranges.size() << endl;
    cout << "Bytes per packing: " << packed_size * sizeof(PackedStateEntry) << endl;
}
