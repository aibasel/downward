#ifndef PER_STATE_BITSET_INFORMATION_H
#define PER_STATE_BITSET_INFORMATION_H

#include "global_state.h"
#include "per_state_information.h"
#include "per_state_array.h"

#include "utils/collections.h"

#include <cassert>
#include <iterator>
#include <unordered_map>


// TODO: avoid duplication with dynamic bitset
template<typename Block = unsigned int>
class BitsetMath {
    static_assert(
        !std::numeric_limits<Block>::is_signed,
        "Block type must be unsigned");
public:
    static const Block zeros;
    static const Block ones;

    static const int bits_per_block = std::numeric_limits<Block>::digits;

    static int compute_num_blocks(std::size_t num_bits) {
        return num_bits / bits_per_block +
               static_cast<int>(num_bits % bits_per_block != 0);
    }

    static std::size_t block_index(std::size_t pos) {
        return pos / bits_per_block;
    }

    static std::size_t bit_index(std::size_t pos) {
        return pos % bits_per_block;
    }

    static Block bit_mask(std::size_t pos) {
        return Block(1) << bit_index(pos);
    }
};


class BitsetView {
    unsigned int *const p;
    const std::size_t array_size;
    const int int_array_size;
public:
    BitsetView(unsigned int *p, const std::size_t size, const int int_array_size);
    BitsetView &operator=(const std::vector<bool> &data);
    BitsetView &operator=(const BitsetView &data);
    void set(std::size_t index);
    void reset(std::size_t index);
    void reset();
    bool test(std::size_t index) const;
    void intersect(const BitsetView &other);
    int size() const;
};


// TODO: update usage example 2 in state_registry.h

class PerStateBitset : public PerStateInformationBase {
    int bitset_size;
    // TODO rename?
    int int_array_size;
    PerStateArray<unsigned int> data;


    /*
      Returns the SegmentedArrayVector associated with the given StateRegistry.
      If no vector is associated with this registry yet, an empty one is created.
      Both the registry and the returned vector are cached to speed up
      consecutive calls with the same registry.
    */
    segmented_vector::SegmentedArrayVector<unsigned int> *get_entries(const StateRegistry *registry);

    /*
      Returns the SegmentedArrayVector associated with the given StateRegistry.
      Returns nullptr, if no vector is associated with this registry yet.
      Otherwise, both the registry and the returned vector are cached to speed
      up consecutive calls with the same registry.
    */
    const segmented_vector::SegmentedArrayVector<unsigned int> *get_entries(const StateRegistry *registry) const;

public:
    PerStateBitset(int array_size_);
    explicit PerStateBitset(const std::vector<bool> &default_value);

    // No implementation to forbid copies and assignment
    PerStateBitset(const PerStateBitset &) = delete;
    PerStateBitset &operator=(const PerStateBitset &) = delete;

    BitsetView operator[](const GlobalState &state);
    void remove_state_registry(StateRegistry *registry);
};


template<typename Block>
const Block BitsetMath<Block>::zeros = Block(0);

template<typename Block>
// MSVC's bitwise negation always returns int.
const Block BitsetMath<Block>::ones = Block(~Block(0));

#endif
