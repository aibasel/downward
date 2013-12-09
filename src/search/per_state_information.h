#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "globals.h"
#include "segmented_vector.h"
#include "state.h"
#include "state_id.h"
#include "state_registry.h"
#include "utilities.h"

#include <cassert>
#include <iterator>

#include <hash_map>

class StateRegistry;

template<class Entry>
class PerStateInformation {
private:
    const Entry default_value;
    typedef __gnu_cxx::hash_map<const StateRegistry *,
        SegmentedVector<Entry>, hash_pointer > EntryVectorMap;
    EntryVectorMap entries_by_registry;

    const StateRegistry *cached_registry;
    SegmentedVector<Entry> *cached_entries;

    void set_cached_registry(const StateRegistry *registry) {
        if (cached_registry != registry) {
            cached_registry = registry;
            cached_entries = &entries_by_registry[registry];
        }
    }

public:
    // TODO this iterates over StateIDs not over entries. Move it to StateRegistry?
    //      A better implementation would allow to iterate over pair<StateID, Entry>.
    class const_iterator : public std::iterator<std::forward_iterator_tag,
                                                StateID> {
        friend class PerStateInformation<Entry>;
        const PerStateInformation<Entry> &owner;
        const StateRegistry *registry;
        StateID pos;

        const_iterator(const PerStateInformation<Entry> &owner_,
                       const StateRegistry *registry_, size_t start)
            : owner(owner_), registry(registry_), pos(start) {}
    public:
        const_iterator(const const_iterator& other)
            : owner(other.owner), registry(other.registry), pos(other.pos) {}

        ~const_iterator() {}

        const_iterator& operator++() {
            ++pos.value;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const const_iterator& rhs) {
            return &owner == &rhs.owner && registry == rhs.registry && pos == rhs.pos;
        }

        bool operator!=(const const_iterator& rhs) {
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

    PerStateInformation()
        : default_value(),
          cached_registry(0),
          cached_entries(0) {
    }

    PerStateInformation(const Entry &default_value_)
        : default_value(default_value_),
          cached_registry(0),
          cached_entries(0) {
    }

    Entry &operator[](const State &state) {
        set_cached_registry(state.get_registry());
        assert(cached_registry == state.get_registry());
        int state_id = state.get_id().value;
        size_t virtual_size = cached_registry->size();
        assert(state_id >= 0 && state_id < virtual_size);
        if (cached_entries->size() < virtual_size) {
            cached_entries->resize(virtual_size, default_value);
        }
        return (*cached_entries)[state_id];
    }

    const Entry &operator[](const State &state) const {
        int state_id = state.get_id().value;
        const StateRegistry *registry = state.get_registry();
        const SegmentedVector<Entry> *entries;
        if (cached_registry == registry) {
            entries = cached_entries;
        } else {
            typename EntryVectorMap::const_iterator it = entries_by_registry.find(registry);
            if (it == entries_by_registry.end()) {
                return default_value;
            } else {
                entries = &it->second;
            }
        }
        assert(state_id >= 0 && state_id < registry->size());
        if (state_id >= entries->size()) {
            return default_value;
        }
        return (*entries)[state_id];
    }
};

#endif
