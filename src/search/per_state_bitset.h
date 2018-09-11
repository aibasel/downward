#ifndef PER_STATE_BITSET_INFORMATION_H
#define PER_STATE_BITSET_INFORMATION_H

#include "per_state_array.h"

#include <vector>


class BitsetMath {
public:
    using Block = unsigned int;
    static_assert(
        !std::numeric_limits<Block>::is_signed,
        "Block type must be unsigned");

    static const Block zeros = Block(0);
    // MSVC's bitwise negation always returns int.
    static const Block ones = Block(~Block(0));
    static const int bits_per_block = std::numeric_limits<Block>::digits;

    static int compute_num_blocks(std::size_t num_bits);
    static std::size_t block_index(std::size_t pos);
    static std::size_t bit_index(std::size_t pos);
    static Block bit_mask(std::size_t pos);
};


class BitsetView {
    ArrayView<BitsetMath::Block> data;
    int num_bits;
public:
    BitsetView(ArrayView<BitsetMath::Block> data, int num_bits);
    BitsetView(const BitsetView &other);
    BitsetView &operator=(const BitsetView &other);
    void set(int index);
    void reset(int index);
    void reset();
    bool test(int index) const;
    void intersect(const BitsetView &other);
    int size() const;
};


// TODO: update usage example 2 in state_registry.h

class PerStateBitset : public PerStateInformationBase {
    int num_blocks;
    PerStateArray<BitsetMath::Block> data;

public:
    explicit PerStateBitset(const std::vector<bool> &default_bits);

    PerStateBitset(const PerStateBitset &) = delete;
    PerStateBitset &operator=(const PerStateBitset &) = delete;

    BitsetView operator[](const GlobalState &state);
    void remove_state_registry(StateRegistry *registry);
};

#endif
