#include "per_state_bitset_information.h"

#include <climits> // used for calculating bitsize of int
#include <cmath> // used for rounding up int_array_size


size_t INT_BITSIZE =  sizeof(int) * CHAR_BIT;

BitsetView::BitsetView(unsigned int *p, size_t size) : p(p), array_size(size), int_array_size(std::ceil(double(array_size)/INT_BITSIZE)) {}
// TODO: is return *this correct?
BitsetView &BitsetView::operator=(const std::vector<bool> &data) {
    assert(data.size() == array_size);
    size_t pos = 0;
    for(size_t i = 0; i < int_array_size; ++i) {
        for(size_t j = 0; j < INT_BITSIZE; ++j) {
            if(data[pos]) {
                p[i] += ((unsigned int)(1) << j); // TODO: is this correct?
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
    for(size_t i = 0; i < int_array_size; ++i) {
        p[i] = data.p[i];
    }
    return *this;
}

void BitsetView::set(int index) {
    assert(index < (int) array_size);
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    p[array_pos] |= ((unsigned int)(1) << offset);
}

void BitsetView::reset(int index) {
    assert(index < (int) array_size);
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    p[array_pos] &= ~((unsigned int)(1) << offset);
}

bool BitsetView::test(int index) const {
    assert(index < (int) array_size);
    int array_pos = index / INT_BITSIZE;
    int offset = index % INT_BITSIZE;
    return ( (p[array_pos] & ((unsigned int)(1) << offset)) != 0 );
}

void BitsetView::intersect(BitsetView &other) {
    assert(array_size == other.array_size);
    for(size_t i = 0; i < int_array_size; ++i) {
        p[i] &= other.p[i];
    }
}

size_t BitsetView::size() const {
    return array_size;
}




segmented_vector::SegmentedArrayVector<unsigned int> *PerStateBitsetInformation::get_entries(const StateRegistry *registry) {
    if (cached_registry != registry) {
        cached_registry = registry;
        typename EntryArrayVectorMap::const_iterator it = entry_arrays_by_registry.find(registry);
        if (it == entry_arrays_by_registry.end()) {
            cached_entries = new segmented_vector::SegmentedArrayVector<unsigned int>(int_array_size);
            entry_arrays_by_registry[registry] = cached_entries;
            registry->subscribe(this);
        } else {
            cached_entries = it->second;
        }
    }
    assert(cached_registry == registry && cached_entries == entry_arrays_by_registry[registry]);
    return cached_entries;
}

const segmented_vector::SegmentedArrayVector<unsigned int> *PerStateBitsetInformation::get_entries(const StateRegistry *registry) const {
    if (cached_registry != registry) {
        typename EntryArrayVectorMap::const_iterator it = entry_arrays_by_registry.find(registry);
        if (it == entry_arrays_by_registry.end()) {
            return 0;
        } else {
            cached_registry = registry;
            cached_entries = const_cast<segmented_vector::SegmentedArrayVector<unsigned int> *>(it->second);
        }
    }
    assert(cached_registry == registry);
    return cached_entries;
}

PerStateBitsetInformation::PerStateBitsetInformation(size_t array_size_)
    : array_size(array_size_),
      int_array_size(std::ceil(double(array_size)/INT_BITSIZE)),
      default_array(std::vector<unsigned int>(int_array_size,0)),
      cached_registry(0),
      cached_entries(0) {
}

PerStateBitsetInformation::PerStateBitsetInformation(size_t array_size_, const std::vector<bool> &default_array_)
    : array_size(array_size_),
      int_array_size(std::ceil(double(array_size)/INT_BITSIZE)),
      default_array(std::vector<unsigned int>(int_array_size,0)),
      cached_registry(0),
      cached_entries(0) {
    assert(default_array_.size() == array_size);
    std::vector<unsigned int> tmp = std::vector<unsigned int>(int_array_size,0);
    size_t pos = 0;
    for(size_t i = 0; i < int_array_size; ++i) {
        for(size_t j = 0; j < INT_BITSIZE; ++j) {
            if(default_array_[pos]) {
                default_array[i] += ((unsigned int)(1) << j); // TODO: is this correct?
            }
            if(pos == array_size-1) {
                break;
            } else {
                pos++;
            }
        }
    }
}

PerStateBitsetInformation::~PerStateBitsetInformation() {
    for (typename EntryArrayVectorMap::iterator it = entry_arrays_by_registry.begin();
         it != entry_arrays_by_registry.end(); ++it) {
        it->first->unsubscribe(this);
        delete it->second;
    }
}

BitsetView PerStateBitsetInformation::operator[](const GlobalState &state) {
    const StateRegistry *registry = &state.get_registry();
    segmented_vector::SegmentedArrayVector<unsigned int> *entries = get_entries(registry);
    int state_id = state.get_id().value;
    size_t virtual_size = registry->size();
    assert(utils::in_bounds(state_id, *registry));
    if (entries->size() < virtual_size) {
        entries->resize(virtual_size, &default_array[0]);
    }
    // TODO: is this effiecient? is move better?
    return BitsetView((*cached_entries)[state_id], array_size);
}

const BitsetView PerStateBitsetInformation::operator[](const GlobalState &state) const {
    const StateRegistry *registry = &state.get_registry();
    const segmented_vector::SegmentedArrayVector<unsigned int> *entries = get_entries(registry);
    if (!entries) {
        return BitsetView(nullptr,0); // TODO: deal with this in a better way
    }
    int state_id = state.get_id().value;
    assert(utils::in_bounds(state_id, *registry));
    int num_entries = entries->size();
    if (state_id >= num_entries) {
        return BitsetView(nullptr,0); // TODO: deal with this in a better way
    }
    // TODO: is this effiecient? is move better?
    return BitsetView((*cached_entries)[state_id], array_size);
}

void PerStateBitsetInformation::remove_state_registry(StateRegistry *registry) {
    delete entry_arrays_by_registry[registry];
    entry_arrays_by_registry.erase(registry);
    if (registry == cached_registry) {
        cached_registry = 0;
        cached_entries = 0;
    }
}
