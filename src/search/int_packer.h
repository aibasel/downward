#ifndef INT_PACKER_H
#define INT_PACKER_H

#include <vector>

// TODO: Make name more generic and try to hide it in IntPacker class (issue348).
typedef unsigned int PackedStateEntry;

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
    // Note that signed ints limit the ranges to 31 bits.
    explicit IntPacker(const std::vector<int> &ranges);
    ~IntPacker();

    int get(const PackedStateEntry *buffer, int var) const;
    void set(PackedStateEntry *buffer, int var, int value) const;

    int get_num_bins() const {return num_bins; }
    int get_bin_size_in_bytes() const {return sizeof(PackedStateEntry); }
};

#endif
