#ifndef INT_PACKER_H
#define INT_PACKER_H

#include "packed_state_entry.h"

#include <vector>

class IntPacker {
    struct PackedEntry {
        int index;
        int shift;
        PackedStateEntry read_mask;
        PackedStateEntry clear_mask;
    };

    std::vector<PackedEntry> packed_entries;
    int num_bins;

    void pack_bins(const std::vector<int> &ranges);

    // No implementation to prevent default construction.
    IntPacker();
public:
    explicit IntPacker(const std::vector<int> &ranges);
    ~IntPacker();

    int get(const PackedStateEntry *buffer, int var) const;
    void set(PackedStateEntry *buffer, int var, int value) const;

    int get_num_bins() const {return num_bins; }
};

#endif
