#ifndef PER_STATE_ARRAY_INFORMATION_H
#define PER_STATE_ARRAY_INFORMATION_H

#include "global_state.h"
#include "globals.h"
#include "per_state_information.h"
#include "segmented_vector.h"
#include "state_id.h"
#include "state_registry.h"

#include "utils/collections.h"

#include <cassert>
#include <iterator>
#include <unordered_map>

template<class T>
class ArrayView {
    T *p;
    size_t size;
public:
    // we might want to make the constructor private and
    // PerStateArraInformation a friend class in the future
    ArrayView(T *p, size_t size) : p(p), size(size) {}
    ArrayView<T> &operator=(const std::vector<T> &data) {
        assert(data.size() == size);
        for(T &e : data) {
            *p = e;
            p++;
        }
    }
    ArrayView<T> &operator=(const ArrayView<T> &data) {
        assert(data.size == size);
        for(size_t i = 0; i < size; ++i) {
            p[i] = data.p[i];
        }
    }

    std::vector<T> get_vector() {
        std::vector<T> ret(size);
        for(size_t i = 0; i < size; ++i) {
            ret[i] = p[i];
        }
        return std::move(ret);
    }

    T &operator[](int index) {
        return p[index];
    }

    const T &operator[](int index) const {
        return p[index];
    }
};

/*
  PerStateInformation is used to associate information with states.
  PerStateInformation<Entry> logically behaves somewhat like an unordered map
  from states to objects of class Entry. However, lookup of unknown states is
  supported and leads to insertion of a default value (similar to the
  defaultdict class in Python).

  For example, landmark heuristics can use it to store the set of landmarks
  with every state reached during search, or search algorithms can use it to
  associate g values or creating operators with a state.

  Implementation notes: PerStateInformation is essentially implemented as a
  kind of two-level map:
    1. Find the correct SegmentedVector for the registry of the given state.
    2. Look up the associated entry in the SegmentedVector based on the ID of
       the state.
  It is common in many use cases that we look up information for states from
  the same registry in sequence. Therefore, to make step 1. more efficient, we
  remember (in "cached_registry" and "cached_entries") the results of the
  previous lookup and reuse it on consecutive lookups for the same registry.

  A PerStateInformation object subscribes to every StateRegistry for which it
  stores information. Once a StateRegistry is destroyed, it notifies all
  subscribed objects, which in turn destroy all information stored for states
  in that registry.
*/

template<class Element>
class PerStateArrayInformation : public PerStateInformationBase {
    size_t array_size;
    const std::vector<Element> default_array;
    typedef std::unordered_map<const StateRegistry *,
                               SegmentedArrayVector<Element> * > EntryArrayVectorMap;
    EntryArrayVectorMap entry_arrays_by_registry;

    mutable const StateRegistry *cached_registry;
    mutable SegmentedArrayVector<Element> *cached_entries;

    /*
      Returns the SegmentedVector associated with the given StateRegistry.
      If no vector is associated with this registry yet, an empty one is created.
      Both the registry and the returned vector are cached to speed up
      consecutive calls with the same registry.
    */
    SegmentedArrayVector<Element> *get_entries(const StateRegistry *registry) {
        if (cached_registry != registry) {
            cached_registry = registry;
            typename EntryArrayVectorMap::const_iterator it = entry_arrays_by_registry.find(registry);
            if (it == entry_arrays_by_registry.end()) {
                cached_entries = new SegmentedArrayVector<Element>(array_size);
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
      Returns the SegmentedVector associated with the given StateRegistry.
      Returns 0, if no vector is associated with this registry yet.
      Otherwise, both the registry and the returned vector are cached to speed
      up consecutive calls with the same registry.
    */
    const SegmentedArrayVector<Element> *get_entries(const StateRegistry *registry) const {
        if (cached_registry != registry) {
            typename EntryArrayVectorMap::const_iterator it = entry_arrays_by_registry.find(registry);
            if (it == entry_arrays_by_registry.end()) {
                return 0;
            } else {
                cached_registry = registry;
                cached_entries = const_cast<SegmentedArrayVector<Element> *>(it->second);
            }
        }
        assert(cached_registry == registry);
        return cached_entries;
    }

    // No implementation to forbid copies and assignment
    PerStateArrayInformation(const PerStateArrayInformation<Element> &);
    PerStateArrayInformation &operator=(const PerStateArrayInformation<Element> &);
public:
    // TODO this iterates over StateIDs not over entries. Move it to StateRegistry?
    //      A better implementation would allow to iterate over pair<StateID, Entry>.
    class const_iterator : public std::iterator<std::forward_iterator_tag,
                                                StateID> {
        friend class PerStateArrayInformation<Element>;
        const PerStateArrayInformation<Element> &owner;
        const StateRegistry *registry;
        StateID pos;

        const_iterator(const PerStateArrayInformation<Element> &owner_,
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

    const_iterator begin(const StateRegistry *registry) const {
        return const_iterator(*this, registry, 0);
    }
    const_iterator end(const StateRegistry *registry) const {
        return const_iterator(*this, registry, registry->size());
    }

    PerStateArrayInformation(size_t array_size_)
        : array_size(array_size_),
          default_array(std::vector<Element>(array_size_)),
          cached_registry(0),
          cached_entries(0) {
    }

    explicit PerStateArrayInformation(size_t array_size_, const std::vector<Element> &default_array_)
        : array_size(array_size_),
          default_array(default_array_),
          cached_registry(0),
          cached_entries(0) {
        assert(default_array.size() == array_size);
    }

    ~PerStateArrayInformation() {
        for (typename EntryArrayVectorMap::iterator it = entry_arrays_by_registry.begin();
             it != entry_arrays_by_registry.end(); ++it) {
            it->first->unsubscribe(this);
            delete it->second;
        }
    }

    ArrayView<Element> operator[](const GlobalState &state) {
        const StateRegistry *registry = &state.get_registry();
        SegmentedArrayVector<Element> *entries = get_entries(registry);
        int state_id = state.get_id().value;
        size_t virtual_size = registry->size();
        assert(utils::in_bounds(state_id, *registry));
        if (entries->size() < virtual_size) {
            entries->push_back(&default_array[0]);
        }
        // TODO: is this effiecient? is move better?
        return ArrayView<Element>((*cached_entries)[state_id], array_size);
    }

    const ArrayView<Element> operator[](const GlobalState &state) const {
        const StateRegistry *registry = &state.get_registry();
        const SegmentedArrayVector<Element> *entries = get_entries(registry);
        if (!entries) {
            return nullptr;
        }
        int state_id = state.get_id().value;
        assert(utils::in_bounds(state_id, *registry));
        int num_entries = entries->size();
        if (state_id >= num_entries) {
            return nullptr;
        }
        // TODO: is this effiecient? is move better?
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
