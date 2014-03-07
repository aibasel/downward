#ifndef INT_PACKER_H
#define INT_PACKER_H

#include "packed_state_entry.h"

#include <vector>

class IntPacker {
    struct PackedInt {
        int index;
        int shift;
        PackedStateEntry read_mask;
        PackedStateEntry clear_mask;
    };

    std::vector<PackedInt> packed_ints;
    int packed_size;

    void calculate_packed_size(const std::vector<int> &ranges);

    // No implementation to prevent default construction.
    IntPacker();
public:
    explicit IntPacker(const std::vector<int> &ranges);
    ~IntPacker();

    int get(const PackedStateEntry *buffer, int var) const;
    void set(PackedStateEntry *buffer, int var, int value) const;

    int get_packed_size() const {return packed_size; }
};

#endif
