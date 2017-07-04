#include "per_state_bitset.h"

#include <climits>
#include <cmath>


BitsetView::BitsetView(unsigned int *p, const std::size_t size, const int int_array_size) :
    p(p), array_size(size), int_array_size(int_array_size) {}

BitsetView &BitsetView::operator=(const std::vector<bool> &data) {
    assert(data.size() == array_size);
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

void BitsetView::set(std::size_t index) {
    assert(index < array_size);
    int block_index = BitsetMath<unsigned int>::block_index(index);
    p[block_index] |= BitsetMath<unsigned int>::bit_mask(index);
}

void BitsetView::reset(std::size_t index) {
    assert(index < array_size);
    int block_index = BitsetMath<unsigned int>::block_index(index);
    p[block_index] &= ~BitsetMath<unsigned int>::bit_mask(index);
}

void BitsetView::reset() {
    std::fill(&p[0], &p[int_array_size], BitsetMath<unsigned int>::zeros);
}

bool BitsetView::test(std::size_t index) const {
    assert(index < array_size);
    int block_index = BitsetMath<unsigned int>::block_index(index);
    return (p[block_index] & BitsetMath<unsigned int>::bit_mask(index)) != 0;
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

PerStateBitset::PerStateBitset(int array_size)
    : bitset_size(array_size),
      int_array_size(BitsetMath<unsigned int>::compute_num_blocks(array_size)),
      data(int_array_size) {
}

std::vector<unsigned int> pack_bit_vector(const std::vector<bool> &bits) {
    int num_bits = bits.size();
    int num_blocks = BitsetMath<unsigned int>::compute_num_blocks(num_bits);
    std::vector<unsigned int> packed_bits = std::vector<unsigned int>(num_blocks, 0);
    BitsetView bitset_view = BitsetView(packed_bits.data(), num_bits, num_blocks);
    for(int i = 0; i < num_bits; ++i) {
        if(bits[i]) {
            bitset_view.set(i);
        }
    }
    return packed_bits;
}

PerStateBitset::PerStateBitset(const std::vector<bool> &default_value)
    : bitset_size(default_value.size()),
      int_array_size(BitsetMath<unsigned int>::compute_num_blocks(bitset_size)),
      data(int_array_size, pack_bit_vector(default_value)) {
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
