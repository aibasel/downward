#ifndef PER_STATE_BITSET_H
#define PER_STATE_BITSET_H

#include "per_state_array.h"
#include "algorithms/dynamic_bitset.h"

#include <vector>


class BitsetMath {
public:
    using Block = unsigned int;
    static_assert(
        !std::numeric_limits<Block>::is_signed,
        "Block type must be unsigned");

    static const Block zeros = Block(0);
    // MSVC's bitwise negation always returns a signed type.
    static const Block ones = Block(~Block(0));
    static const int bits_per_block = std::numeric_limits<Block>::digits;

    static int compute_num_blocks(std::size_t num_bits);
    static std::size_t block_index(std::size_t pos);
    static std::size_t bit_index(std::size_t pos);
    static Block bit_mask(std::size_t pos);
};


class ConstBitsetView {
    ConstArrayView<BitsetMath::Block> data;
    int num_bits;
public:
    ConstBitsetView(ConstArrayView<BitsetMath::Block> data, int num_bits) :
        data(data), num_bits(num_bits) {}


    ConstBitsetView(const ConstBitsetView &other) = default;
    ConstBitsetView &operator=(const ConstBitsetView &other) = default;

    bool test(int index) const;
    int size() const;
};


class BitsetView {

    friend class dynamic_bitset::DynamicBitset<BitsetMath::Block>;

    ArrayView<BitsetMath::Block> data;
    int num_bits;
public:
    BitsetView(ArrayView<BitsetMath::Block> data, int num_bits) :
        data(data), num_bits(num_bits) {}


    BitsetView(const BitsetView &other) = default;
    BitsetView &operator=(const BitsetView &other) = default;

    operator ConstBitsetView() const {
        return ConstBitsetView(data, num_bits);
    }

    void set(int index);
    void reset(int index);
    void reset();
    bool test(int index) const;
    void intersect(const BitsetView &other);
    int size() const;
    int count() const;
    bool any() const;
    void copy_from(const BitsetView& other);
    void update_not();
    void update_and(const BitsetView &other);
    void update_or(const BitsetView &other);
    void update_andc(const BitsetView &other);
    void update_orc(const BitsetView &other);
    void update_xor(const BitsetView &other);
    void copy_from(const dynamic_bitset::DynamicBitset<BitsetMath::Block>& other);
    void update_and(const dynamic_bitset::DynamicBitset<BitsetMath::Block> &other);
    void update_or(const dynamic_bitset::DynamicBitset<BitsetMath::Block> &other);
    void update_andc(const dynamic_bitset::DynamicBitset<BitsetMath::Block> &other);
    void update_orc(const dynamic_bitset::DynamicBitset<BitsetMath::Block> &other);
    void update_xor(const dynamic_bitset::DynamicBitset<BitsetMath::Block> &other);
    template<class T>
    BitsetView& operator&=(const T &other);
    template<class T>
    BitsetView& operator|=(const T &other);
    template<class T>
    BitsetView& operator^=(const T &other);
};

BitsetView operator~(BitsetView copy);
template<class T>
BitsetView operator&&(BitsetView copy, const T &other);
template<class T>
BitsetView operator||(BitsetView copy, const T &other);
template<class T>
BitsetView operator^(BitsetView copy, const T &other);


class PerStateBitset {
    int num_bits_per_entry;
    PerStateArray<BitsetMath::Block> data;
public:
    explicit PerStateBitset(const std::vector<bool> &default_bits);

    PerStateBitset(const PerStateBitset &) = delete;
    PerStateBitset &operator=(const PerStateBitset &) = delete;

    BitsetView operator[](const State &state);
    ConstBitsetView operator[](const State &state) const;
};

#endif
