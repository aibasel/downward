#include "per_state_bitset.h"

using namespace std;


int BitsetMath::compute_num_blocks(size_t num_bits) {
    return num_bits / bits_per_block +
            static_cast<int>(num_bits % bits_per_block != 0);
}

size_t BitsetMath::block_index(size_t pos) {
    return pos / bits_per_block;
}

size_t BitsetMath::bit_index(size_t pos) {
    return pos % bits_per_block;
}

BitsetMath::Block BitsetMath::bit_mask(size_t pos) {
    return Block(1) << bit_index(pos);
}


BitsetView::BitsetView(ArrayView<BitsetMath::Block> data, int num_bits) :
    data(data), num_bits(num_bits) {}

BitsetView::BitsetView(const BitsetView &other)
    : data(other.data), num_bits(other.num_bits) {}

BitsetView &BitsetView::operator=(const BitsetView &other) {
    data = other.data;
    num_bits = other.num_bits;
    return *this;
}

void BitsetView::set(int index) {
    assert(index >= 0 && index < array_size);
    int block_index = BitsetMath::block_index(index);
    data[block_index] |= BitsetMath::bit_mask(index);
}

void BitsetView::reset(int index) {
    assert(index >= 0 && index < array_size);
    int block_index = BitsetMath::block_index(index);
    data[block_index] &= ~BitsetMath::bit_mask(index);
}

// TODO: can we use fill here? Not sure what ArrayView needs to return for begin and end
void BitsetView::reset() {
    for (int i = 0; i < data.size(); ++i) {
        data[i] = BitsetMath::zeros;
    }
}

bool BitsetView::test(int index) const {
    assert(index >= 0 && index < array_size);
    int block_index = BitsetMath::block_index(index);
    return (data[block_index] & BitsetMath::bit_mask(index)) != 0;
}

void BitsetView::intersect(const BitsetView &other) {
    assert(array_size == other.array_size);
    for (int i = 0; i < data.size(); ++i) {
        data[i] &= other.data[i];
    }
}

int BitsetView::size() const {
    return num_bits;
}


static vector<unsigned int> pack_bit_vector(const vector<bool> &bits) {
    int num_bits = bits.size();
    int num_blocks = BitsetMath::compute_num_blocks(num_bits);
    vector<unsigned int> packed_bits = vector<unsigned int>(num_blocks, 0);
    // TODO: this looks ugly
    BitsetView bitset_view =
        BitsetView(ArrayView<unsigned int>(packed_bits.data(), num_blocks), num_bits);
    for (int i = 0; i < num_bits; ++i) {
        if (bits[i]) {
            bitset_view.set(i);
        }
    }
    return packed_bits;
}

PerStateBitset::PerStateBitset(const vector<bool> &default_bits)
    : num_blocks(BitsetMath::compute_num_blocks(default_bits.size())),
      data(pack_bit_vector(default_bits)) {
}


BitsetView PerStateBitset::operator[](const GlobalState &state) {
    return BitsetView(data[state], num_blocks);
}

// TODO: does this do the right thing?
void PerStateBitset::remove_state_registry(StateRegistry *registry) {
    data.remove_state_registry(registry);
}
