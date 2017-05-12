#ifndef PER_STATE_ARRAY_INFORMATION_H
#define PER_STATE_ARRAY_INFORMATION_H

#include "global_state.h"
#include "globals.h"
#include "per_state_information.h"
#include "per_state_bitset.h"
#include "algorithms/segmented_vector.h"
#include "state_id.h"
#include "state_registry.h"

#include "utils/collections.h"

#include <cassert>
#include <iterator>
#include <unordered_map>

template<class T>
class ArrayView {
    template<typename>
    friend class PerStateArray;
    T *const p;
    const int array_size;
    ArrayView(T *p, size_t size) : p(p), array_size(size) {}
public:
    // TODO: is return *this correct?
    ArrayView<T> &operator=(const std::vector<T> &data) {
        assert(data.size() == array_size);
        for(size_t i = 0; i < data.size; ++i) {
            p[i] = data[i];
        }
        return *this;
    }
    ArrayView<T> &operator=(const ArrayView<T> &data) {
        assert(data.array_size == array_size);
        for(size_t i = 0; i < array_size; ++i) {
            p[i] = data.p[i];
        }
        return *this;
    }


    T &operator[](int index) {
        return p[index];
    }

    const T &operator[](int index) const {
        return p[index];
    }

    int size() const {
        return array_size;
    }
};

/*
  PerStateArrayInformation is used to associate information with states.
  PerStateArrayInformation<Entry> logically behaves somewhat like an unordered map
  from states to arrays of class Entry. However, lookup of unknown states is
  supported and leads to insertion of a default value (similar to the
  defaultdict class in Python).

  For example, landmark heuristics can use it to store the set of landmarks
  with every state reached during search, or search algorithms can use it to
  associate g values or creating operators with a state.

  Implementation notes: PerStateInformation is essentially implemented as a
  kind of two-level map:
    1. Find the correct SegmentedArrayVector for the registry of the given state.
    2. Look up the associated entry in the SegmentedArrayVector based on the ID of
       the state.
  It is common in many use cases that we look up information for states from
  the same registry in sequence. Therefore, to make step 1. more efficient, we
  remember (in "cached_registry" and "cached_entries") the results of the
  previous lookup and reuse it on consecutive lookups for the same registry.

  A PerStateArrayInformation object subscribes to every StateRegistry for which it
  stores information. Once a StateRegistry is destroyed, it notifies all
  subscribed objects, which in turn destroy all information stored for states
  in that registry.
*/

template<class Element>
class PerStateArray : public PerStateInformationBase {
    friend class PerStateBitset;
    size_t array_size;
    const std::vector<Element> default_array;
    typedef std::unordered_map<const StateRegistry *,
                               segmented_vector::SegmentedArrayVector<Element> * > EntryArrayVectorMap;
    EntryArrayVectorMap entry_arrays_by_registry;

    mutable const StateRegistry *cached_registry;
    mutable segmented_vector::SegmentedArrayVector<Element> *cached_entries;

    /*
      Returns the SegmentedArrayVector associated with the given StateRegistry.
      If no vector is associated with this registry yet, an empty one is created.
      Both the registry and the returned vector are cached to speed up
      consecutive calls with the same registry.
    */
    segmented_vector::SegmentedArrayVector<Element> *get_entries(const StateRegistry *registry) {
        if (cached_registry != registry) {
            cached_registry = registry;
            typename EntryArrayVectorMap::const_iterator it = entry_arrays_by_registry.find(registry);
            if (it == entry_arrays_by_registry.end()) {
                cached_entries = new segmented_vector::SegmentedArrayVector<Element>(array_size);
                entry_arrays_by_registry[registry] = cached_entries;
                registry->subscribe(this);
            } else {
                cached_entries = it->second;
            }
        }
        assert(cached_registry == registry && cached_entries == entry_arrays_by_registry[registry]);
        return cached_entries;
    }

    /*
      Returns the SegmentedArrayVector associated with the given StateRegistry.
      Returns 0, if no vector is associated with this registry yet.
      Otherwise, both the registry and the returned vector are cached to speed
      up consecutive calls with the same registry.
    */
    const segmented_vector::SegmentedArrayVector<Element> *get_entries(const StateRegistry *registry) const {
        if (cached_registry != registry) {
            typename EntryArrayVectorMap::const_iterator it = entry_arrays_by_registry.find(registry);
            if (it == entry_arrays_by_registry.end()) {
                return 0;
            } else {
                cached_registry = registry;
                cached_entries = const_cast<segmented_vector::SegmentedArrayVector<Element> *>(it->second);
            }
        }
        assert(cached_registry == registry);
        return cached_entries;
    }

    // No implementation to forbid copies and assignment
    PerStateArray(const PerStateArray<Element> &);
    PerStateArray &operator=(const PerStateArray<Element> &);

public:
    PerStateArray(size_t array_size_)
        : array_size(array_size_),
          default_array(std::vector<Element>(array_size_)),
          cached_registry(0),
          cached_entries(0) {
    }

    explicit PerStateArray(size_t array_size_, const std::vector<Element> &default_array_)
        : array_size(array_size_),
          default_array(default_array_),
          cached_registry(0),
          cached_entries(0) {
        assert(default_array.size() == array_size);
    }

    ~PerStateArray() {
        for (typename EntryArrayVectorMap::iterator it = entry_arrays_by_registry.begin();
             it != entry_arrays_by_registry.end(); ++it) {
            it->first->unsubscribe(this);
            delete it->second;
        }
    }

    ArrayView<Element> operator[](const GlobalState &state) {
        const StateRegistry *registry = &state.get_registry();
        segmented_vector::SegmentedArrayVector<Element> *entries = get_entries(registry);
        int state_id = state.get_id().value;
        size_t virtual_size = registry->size();
        assert(utils::in_bounds(state_id, *registry));
        if (entries->size() < virtual_size) {
            entries->push_back(&default_array[0]);
        }
        return ArrayView<Element>((*cached_entries)[state_id], array_size);
    }

    void remove_state_registry(StateRegistry *registry) {
        delete entry_arrays_by_registry[registry];
        entry_arrays_by_registry.erase(registry);
        if (registry == cached_registry) {
            cached_registry = 0;
            cached_entries = 0;
        }
    }
};

#endif
