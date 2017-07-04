#ifndef PER_STATE_BITSET_INFORMATION_H
#define PER_STATE_BITSET_INFORMATION_H

#include "global_state.h"
#include "per_state_information.h"
#include "per_state_array.h"

#include "utils/collections.h"

#include <cassert>
#include <iterator>
#include <unordered_map>

class BitsetView {
    unsigned int *const p;
    const int array_size;
    const int int_array_size;
public:
    BitsetView(unsigned int *p, const int size, const int int_array_size);
    BitsetView &operator=(const std::vector<bool> &data);
    BitsetView &operator=(const BitsetView &data);
    void set(int index);
    void reset(int index);
    void reset();
    bool test(int index) const;
    void intersect(const BitsetView &other);
    int size() const;
};


// TODO: update usage example 2 in state_registry.h

class PerStateBitset : public PerStateInformationBase {
    int bitset_size;
    // the size of the int array (=ceil(array_size / INT_BITSIZE))
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
    explicit PerStateBitset(int array_size_, const std::vector<bool> &default_array_);

    // No implementation to forbid copies and assignment
    PerStateBitset(const PerStateBitset &) = delete;
    PerStateBitset &operator=(const PerStateBitset &) = delete;

    BitsetView operator[](const GlobalState &state);
    void remove_state_registry(StateRegistry *registry);
};

#endif
