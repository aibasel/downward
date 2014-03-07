#include "int_packer.h"

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

int get_max_fitting_bits(const vector<vector<int> > &bits_to_entries,
                         int available_bits) {
    for (size_t bits = available_bits; bits != 0; --bits) {
        if (!bits_to_entries[bits].empty()) {
            return bits;
        }
    }
    return 0;
}


IntPacker::IntPacker(const vector<int> &ranges)
    : packed_entries(),
      num_bins(0) {
    pack_bins(ranges);
}

IntPacker::~IntPacker() {
}

int IntPacker::get(const PackedStateEntry *buffer, int pos) const {
    const PackedEntry &packed_entry = packed_entries[pos];
    return (buffer[packed_entry.index] & packed_entry.read_mask) >> packed_entry.shift;
}

void IntPacker::set(PackedStateEntry *buffer, int pos, int value) const {
    const PackedEntry &packed_entry = packed_entries[pos];
    PackedStateEntry before = buffer[packed_entry.index];
    buffer[packed_entry.index] = (before & packed_entry.clear_mask) | (value << packed_entry.shift);
}

void IntPacker::pack_bins(const vector<int> &ranges) {
    assert(packed_entries.empty());
    int num_entries = ranges.size();
    packed_entries.resize(num_entries);

    int bits_per_bin = sizeof(PackedStateEntry) * 8;
    vector<vector<int> > bits_to_entries(bits_per_bin + 1);
    for (size_t pos = 0; pos < num_entries; ++pos) {
        int bits = get_needed_bits(ranges[pos]);
        assert(bits < bits_per_bin);
        bits_to_entries[bits].push_back(pos);
    }
    // Greedy strategy: always add the largest fitting entry.
    int num_packed_entries = 0;
    num_bins = 1;
    int remaining_bits = bits_per_bin;
    while (num_packed_entries < num_entries) {
        int bits = get_max_fitting_bits(bits_to_entries, remaining_bits);
        if (bits == 0) {
            // We cannot pack another int into the current word so we add
            // an additional word.
            ++num_bins;
            remaining_bits = bits_per_bin;
            continue;
        }
        // We can pack another entry of size bits into the current word.
        vector<int> &best_fit_entries = bits_to_entries[bits];
        assert(!best_fit_entries.empty());
        int pos = best_fit_entries.back();
        best_fit_entries.pop_back();
        PackedEntry &packed_entry = packed_entries[pos];
        packed_entry.shift = bits_per_bin - remaining_bits;
        packed_entry.index = num_bins - 1;
        packed_entry.read_mask = 0;
        set_bits(packed_entry.read_mask, packed_entry.shift, packed_entry.shift + bits);
        packed_entry.clear_mask = ~packed_entry.read_mask;
        remaining_bits -= bits;
        ++num_packed_entries;
    }
}
