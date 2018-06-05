#include "per_state_bitset.h"

#include <climits>
#include <cmath>

// TODO: is move a good idea here?
BitsetView::BitsetView(ArrayView<Block> data, int num_bits) :
    data(std::move(data)), num_bits(num_bits) {}

BitsetView::BitsetView(const BitsetView &other)
    : data(other.data), num_bits(other.num_bits) {}

BitsetView &BitsetView::operator=(const BitsetView &other) {
    data = other.data;
    num_bits = other.num_bits;
    return *this;
}

void BitsetView::set(std::size_t index) {
    assert(index < array_size);
    int block_index = BitsetMath<unsigned int>::block_index(index);
    data[block_index] |= BitsetMath<unsigned int>::bit_mask(index);
}

void BitsetView::reset(std::size_t index) {
    assert(index < array_size);
    int block_index = BitsetMath<unsigned int>::block_index(index);
    data[block_index] &= ~BitsetMath<unsigned int>::bit_mask(index);
}

// TODO: can we use fill here? Not sure what ArrayView needs to return for begin and end
void BitsetView::reset() {
    for(int i = 0; i < data.size(); ++i) {
        data[i] = BitsetMath<Block>::zeros;
    }
}

bool BitsetView::test(std::size_t index) const {
    assert(index < array_size);
    int block_index = BitsetMath<unsigned int>::block_index(index);
    return (data[block_index] & BitsetMath<unsigned int>::bit_mask(index)) != 0;
}

void BitsetView::intersect(const BitsetView &other) {
    assert(array_size == other.array_size);
    for(int i = 0; i < data.size(); ++i) {
        data[i] &= other.data[i];
    }
}

int BitsetView::size() const {
    return num_bits;
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
    // TODO: this looks ugly
    BitsetView bitset_view =
            BitsetView(ArrayView<unsigned int>(packed_bits.data(),num_blocks), num_bits);
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


BitsetView PerStateBitset::operator[](const GlobalState &state) {
    return BitsetView(data[state],int_array_size);
}

// TODO: does this do the right thing?
void PerStateBitset::remove_state_registry(StateRegistry *registry) {
    data.remove_state_registry(registry);
}
