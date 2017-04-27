#ifndef PER_STATE_BITSET_INFORMATION_H
#define PER_STATE_BITSET_INFORMATION_H

#include "global_state.h"
#include "globals.h"
#include "per_state_information.h"
#include "algorithms/segmented_vector.h"
#include "algorithms/dynamic_bitset.h"
#include "state_id.h"
#include "state_registry.h"

#include "utils/collections.h"

#include <cassert>
#include <iterator>
#include <unordered_map>

class BitsetView {
    unsigned int *const p;
    const size_t array_size;
    const size_t int_array_size;
public:
    // we might want to make the constructor private and
    // PerStateBitsetInformation a friend class in the future
    BitsetView(unsigned int *p, size_t size);

    BitsetView &operator=(const std::vector<bool> &data);
    BitsetView &operator=(const BitsetView &data);
    void set(int index);
    void reset(int index);
    bool test(int index) const;
    void intersect(BitsetView &other);
    size_t size() const;
};

/*
  PerStateBitsetInformation is a specialization of PerStateArrayInformation<bool>.
  It packs each bool in one bit of an unsigned int array.
*/

class PerStateBitsetInformation : public PerStateInformationBase {
    size_t array_size;
    size_t int_array_size; // the size of the int array (=ceil(array_size / INT_BITSIZE))
    std::vector<unsigned int> default_array; // TODO: this was const before
    typedef std::unordered_map<const StateRegistry *,
                               segmented_vector::SegmentedArrayVector<unsigned int> * > EntryArrayVectorMap;
    EntryArrayVectorMap entry_arrays_by_registry;

    mutable const StateRegistry *cached_registry;
    mutable segmented_vector::SegmentedArrayVector<unsigned int> *cached_entries;

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
    PerStateBitsetInformation(const PerStateBitsetInformation &);
    PerStateBitsetInformation &operator=(const PerStateBitsetInformation &);


    // TODO: can the nested class also be moved to the source file?
public:
    // TODO this iterates over StateIDs not over entries. Move it to StateRegistry?
    //      A better implementation would allow to iterate over pair<StateID, Entry>.
    class const_iterator : public std::iterator<std::forward_iterator_tag,
                                                StateID> {
        friend class PerStateBitsetInformation;
        const PerStateBitsetInformation &owner;
        const StateRegistry *registry;
        StateID pos;

        const_iterator(const PerStateBitsetInformation &owner_,
                       const StateRegistry *registry_, size_t start)
            : owner(owner_), registry(registry_), pos(start) {}
public:
        const_iterator(const const_iterator &other)
            : owner(other.owner), registry(other.registry), pos(other.pos) {}

        ~const_iterator() {}

        const_iterator &operator++() {
            ++pos.value;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const const_iterator &rhs) {
            return &owner == &rhs.owner && registry == rhs.registry && pos == rhs.pos;
        }

        bool operator!=(const const_iterator &rhs) {
            return !(*this == rhs);
        }

        StateID operator*() {
            return pos;
        }

        StateID *operator->() {
            return &pos;
        }
    };

    // TODO; should be in the cc file (when I moved this into the source file,
    // it could not find const_iterator.
    const_iterator begin(const StateRegistry *registry) const {
        return const_iterator(*this, registry, 0);
    }
    const_iterator end(const StateRegistry *registry) const {
        return const_iterator(*this, registry, registry->size());
    }

    PerStateBitsetInformation(size_t array_size_);
    explicit PerStateBitsetInformation(size_t array_size_, const std::vector<bool> &default_array_);
    ~PerStateBitsetInformation();

    BitsetView operator[](const GlobalState &state);

    const BitsetView operator[](const GlobalState &state) const;
    void remove_state_registry(StateRegistry *registry);
};

#endif // PER_STATE_BITSET_INFORMATION_H
