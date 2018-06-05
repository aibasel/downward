#ifndef PER_STATE_ARRAY_INFORMATION_H
#define PER_STATE_ARRAY_INFORMATION_H

#include "global_state.h"
#include "per_state_information.h"

#include <cassert>
#include <iterator>
#include <unordered_map>


template<class T>
class ArrayView {
    T * p;
    int array_size;
public:
    ArrayView(T *p, size_t size) : p(p), array_size(size) {}
    ArrayView(const ArrayView<T> &other) : p(other.p), array_size(other.array_size) {}

    ArrayView<T> &operator=(const ArrayView<T> &other) {
        p = other.p;
        array_size = other.array_size;
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
  from states to equal-length arrays of class Entry. However, lookup of unknown states is
  supported and leads to insertion of a default value (similar to the
  defaultdict class in Python).

  For example, landmark heuristics can use it to store the set of reached landmarks
  for every state encountered during search.

  Implementation notes: PerStateArrayInformation is essentially implemented as a
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
    size_t array_size;
    const std::vector<Element> default_array;
    using EntryArrayVectorMap = std::unordered_map<const StateRegistry *,
                                                   segmented_vector::SegmentedArrayVector<Element> * >;
    // TODO: use unique_ptr
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
            const auto it = entry_arrays_by_registry.find(registry);
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
      Returns nullptr, if no vector is associated with this registry yet.
      Otherwise, both the registry and the returned vector are cached to speed
      up consecutive calls with the same registry.
    */
    const segmented_vector::SegmentedArrayVector<Element> *get_entries(const StateRegistry *registry) const {
        if (cached_registry != registry) {
            const auto it = entry_arrays_by_registry.find(registry);
            if (it == entry_arrays_by_registry.end()) {
                return nullptr;
            } else {
                cached_registry = registry;
                cached_entries = const_cast<segmented_vector::SegmentedArrayVector<Element> *>(it->second);
            }
        }
        assert(cached_registry == registry);
        return cached_entries;
    }

public:

    explicit PerStateArray(size_t array_size, const std::vector<Element> &default_array)
        : array_size(array_size),
          default_array(default_array),
          cached_registry(nullptr),
          cached_entries(nullptr) {
        assert(default_array.size() == array_size);
    }

    PerStateArray(size_t array_size) : PerStateArray(array_size, std::vector<Element>(array_size)) {}

    // No implementation to forbid copies and assignment
    PerStateArray(const PerStateArray<Element> &) = delete;
    PerStateArray &operator=(const PerStateArray<Element> &) = delete;

    // TODO: could we use unique_ptr to avoid naked new and delete?
    ~PerStateArray() {
        for (auto it = entry_arrays_by_registry.begin();
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
            entries->resize(virtual_size, &default_array[0]);
        }
        assert(entries->size() >= virtual_size); // TODO: remove this assert
        return ArrayView<Element>((*entries)[state_id], array_size);
    }

    void remove_state_registry(StateRegistry *registry) {
        delete entry_arrays_by_registry[registry];
        entry_arrays_by_registry.erase(registry);
        if (registry == cached_registry) {
            cached_registry = nullptr;
            cached_entries = nullptr;
        }
    }
};

#endif
