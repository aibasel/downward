#include "per_state_bitset.h"

#include <climits>
#include <cmath>

using Block = unsigned int;
static_assert(
    !std::numeric_limits<Block>::is_signed,
    "Block type must be unsigned");
static const int INT_BITSIZE = std::numeric_limits<Block>::digits;

BitsetView::BitsetView(unsigned int *p, const int size, const int int_array_size) :
    p(p), array_size(size), int_array_size(int_array_size) {}

BitsetView &BitsetView::operator=(const std::vector<bool> &data) {
    assert(static_cast<int>(data.size()) == array_size);
    reset();
    for (int i = 0; i < int_array_size; ++i) {
        if (data[i]) {
            set(i);
        }
    }
    return *this;
}
BitsetView &BitsetView::operator=(const BitsetView &data) {
    assert(data.array_size == array_size);
    for(int i = 0; i < int_array_size; ++i) {
        p[i] = data.p[i];
    }
    return *this;
}

void BitsetView::set(int index) {
    assert(utils::in_bounds(index, *this));
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    p[array_pos] |= (1U << offset);
}

void BitsetView::reset(int index) {
    assert(utils::in_bounds(index, *this));
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    p[array_pos] &= ~(1U << offset);
}

void BitsetView::reset() {
    std::fill(&p[0], &p[int_array_size], 0);
}

bool BitsetView::test(int index) const {
    assert(utils::in_bounds(index, *this));
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    return ( (p[array_pos] & (1U << offset)) != 0 );
}

void BitsetView::intersect(const BitsetView &other) {
    assert(array_size == other.array_size);
    for(int i = 0; i < int_array_size; ++i) {
        p[i] &= other.p[i];
    }
}

int BitsetView::size() const {
    return array_size;
}


segmented_vector::SegmentedArrayVector<unsigned int> *PerStateBitset::get_entries(const StateRegistry *registry) {
    return data.get_entries(registry);
}

const segmented_vector::SegmentedArrayVector<unsigned int> *PerStateBitset::get_entries(const StateRegistry *registry) const {
    return data.get_entries(registry);
}

PerStateBitset::PerStateBitset(int array_size_)
    : bitset_size(array_size_),
      int_array_size(std::ceil(double(bitset_size) / INT_BITSIZE)),
      data(int_array_size) {
}

std::vector<unsigned int> pack_bit_vector(const std::vector<bool> &bits) {
    int num_bits = bits.size();
    int num_blocks = num_bits / bits_per_block +
           static_cast<int>(num_bits % bits_per_block != 0);
    std::vector<unsigned int> packed_bits = std::vector<unsigned int>(num_blocks, 0);
    BitsetView bitset_view = BitsetView(packed_bits.data(), num_bits, num_blocks);
    for(int i = 0; i < num_bits; ++i) {
        if(bits[i]) {
            bitset_view.set(i);
        }
    }
    return packed_bits;
}

PerStateBitset::PerStateBitset(int array_size, const std::vector<bool> &default_array)
    : bitset_size(array_size),
      int_array_size(std::ceil(double(array_size)/INT_BITSIZE)),
      data(int_array_size, pack_bit_vector(default_array)) {
    assert(array_size == static_cast<int>(default_array.size()));
}

// TODO: can we avoid code duplication from PerStateArray here?
BitsetView PerStateBitset::operator[](const GlobalState &state) {
    const StateRegistry *registry = &state.get_registry();
    segmented_vector::SegmentedArrayVector<unsigned int> *entries = data.get_entries(registry);
    int state_id = state.get_id().value;
    size_t virtual_size = registry->size();
    assert(utils::in_bounds(state_id, *registry));
    if (entries->size() < virtual_size) {
        entries->resize(virtual_size, &data.default_array[0]);
    }
    return BitsetView((*entries)[state_id], bitset_size, int_array_size);
}

void PerStateBitset::remove_state_registry(StateRegistry *registry) {
    data.remove_state_registry(registry);
}
