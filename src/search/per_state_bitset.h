#ifndef PER_STATE_BITSET_INFORMATION_H
#define PER_STATE_BITSET_INFORMATION_H

#include "global_state.h"
#include "globals.h"
#include "per_state_information.h"
#include "per_state_array.h"
#include "algorithms/segmented_vector.h"
#include "algorithms/dynamic_bitset.h"
#include "state_id.h"
#include "state_registry.h"

#include "utils/collections.h"

#include <cassert>
#include <iterator>
#include <unordered_map>

class BitsetView {
    friend class PerStateBitset;
    unsigned int *const p;
    const int array_size;
    BitsetView(unsigned int *p, int size);
public:
    BitsetView &operator=(const std::vector<bool> &data);
    BitsetView &operator=(const BitsetView &data);
    void set(int index);
    void reset(int index);
    void reset_all();
    bool test(int index) const;
    void intersect(BitsetView &other);
    int size() const;
};


// TODO: update usage example 2 in state_registry.h

/*
  PerStateBitsetInformation is a specialization of PerStateArrayInformation<bool>.
  It packs each bool in one bit of an unsigned int array.
*/

class PerStateBitset : public PerStateInformationBase {
    int array_size;
    int int_array_size; // the size of the int array (=ceil(array_size / INT_BITSIZE))
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
      Returns 0, if no vector is associated with this registry yet.
      Otherwise, both the registry and the returned vector are cached to speed
      up consecutive calls with the same registry.
    */
    const segmented_vector::SegmentedArrayVector<unsigned int> *get_entries(const StateRegistry *registry) const;

    // No implementation to forbid copies and assignment
    PerStateBitset(const PerStateBitset &);
    PerStateBitset &operator=(const PerStateBitset &);

public:
    PerStateBitset(int array_size_);
    explicit PerStateBitset(int array_size_, const std::vector<bool> &default_array_);
    ~PerStateBitset();

    BitsetView operator[](const GlobalState &state);
    void remove_state_registry(StateRegistry *registry);
};

#endif // PER_STATE_BITSET_INFORMATION_H
