#include "per_state_bitset.h"

#include <climits> // used for calculating bitsize of int
#include <cmath> // used for rounding up int_array_size


int INT_BITSIZE =  sizeof(int) * CHAR_BIT;

BitsetView::BitsetView(unsigned int *p, const int size) : p(p), array_size(size) {}

BitsetView &BitsetView::operator=(const std::vector<bool> &data) {
    assert(data.size() == array_size);
    int int_array_size = std::ceil(static_cast<double>(array_size)/INT_BITSIZE);
    int pos = 0;
    for(int i = 0; i < int_array_size; ++i) {
        for(int j = 0; j < INT_BITSIZE; ++j) {
            if(data[pos]) {
                p[i] += (1U << j); // TODO: is this correct?
            }
            if(pos == array_size-1) {
                break;
            } else {
                pos++;
            }
        }
    }
    return *this;
}
BitsetView &BitsetView::operator=(const BitsetView &data) {
    assert(data.array_size == array_size);
    int int_array_size = std::ceil(static_cast<double>(array_size)/INT_BITSIZE);
    for(int i = 0; i < int_array_size; ++i) {
        p[i] = data.p[i];
    }
    return *this;
}

void BitsetView::set(int index) {
    assert(index >= 0 && index < array_size);
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    p[array_pos] |= (1U << offset);
}

void BitsetView::reset(int index) {
    assert(index >= 0 && index < array_size);
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    p[array_pos] &= ~(1U << offset);
}

void BitsetView::reset_all() {
    int int_array_size = std::ceil(static_cast<double>(array_size)/INT_BITSIZE);
    for(int i = 0; i < int_array_size; ++i) {
        p[i] = 0;
    }
}

bool BitsetView::test(int index) const {
    assert(index >= 0 && index < array_size);
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    return ( (p[array_pos] & (1U << offset)) != 0 );
}

void BitsetView::intersect(BitsetView &other) {
    assert(array_size == other.array_size);
    int int_array_size = std::ceil(static_cast<double>(array_size)/INT_BITSIZE);
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
    return data.get_entries(registry); // TODO: will this call the right method (the one that returns const)?
}

PerStateBitset::PerStateBitset(int array_size_)
    : array_size(array_size_),
      int_array_size(std::ceil(double(array_size)/INT_BITSIZE)),
      data(PerStateArray<unsigned int>(int_array_size)) {
}

std::vector<unsigned int> build_default_array(const std::vector<bool> &default_array, int int_array_size) {
    std::vector<unsigned int> tmp = std::vector<unsigned int>(int_array_size,0);
    int pos = 0;
    for(int i = 0; i < int_array_size; ++i) {
        for(int j = 0; j < INT_BITSIZE; ++j) {
            if(default_array[pos]) {
                tmp[i] += (1U << j);
            }
            if(pos == static_cast<int>(default_array.size())-1) {
                break;
            } else {
                pos++;
            }
        }
    }
    return std::move(tmp);
}

PerStateBitset::PerStateBitset(int array_size_, const std::vector<bool> &default_array_)
    : array_size(array_size_),
      int_array_size(std::ceil(double(array_size)/INT_BITSIZE)),
      data(int_array_size, build_default_array(default_array_, int_array_size)) {
    assert(array_size_ == default_array_.size());
}

PerStateBitset::~PerStateBitset() {} // TODO: is it correct to do nothing here?

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
    // TODO: this was cached_entries before... but shouldnt matter?
    return BitsetView((*entries)[state_id], array_size);
}

void PerStateBitset::remove_state_registry(StateRegistry *registry) {
    data.remove_state_registry(registry);
}
