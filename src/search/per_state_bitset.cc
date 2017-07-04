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
      int_array_size(std::ceil(double(bitset_size)/INT_BITSIZE)),
      data(int_array_size) {
}

std::vector<unsigned int> build_default_array(const std::vector<bool> &default_array,
                                              int int_array_size) {
    int array_size = default_array.size();
    std::vector<unsigned int> tmp = std::vector<unsigned int>(int_array_size,0);
    BitsetView tmp_view = BitsetView(tmp.data(), array_size, int_array_size);
    for(int i = 0; i < array_size; ++i) {
        if(default_array[i]) {
            tmp_view.set(i);
        }
    }
    return tmp;
}

PerStateBitset::PerStateBitset(int array_size, const std::vector<bool> &default_array)
    : bitset_size(array_size),
      int_array_size(std::ceil(double(array_size)/INT_BITSIZE)),
      data(int_array_size, build_default_array(default_array, int_array_size)) {
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
