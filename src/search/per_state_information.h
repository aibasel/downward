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

#include <ext/hash_map>

class PerStateInformationBase {
public:
    virtual void state_registry_discarded(StateRegistry *registry) = 0;
    PerStateInformationBase() {
    }
    virtual ~PerStateInformationBase() {}
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
*/
template<class Entry>
class PerStateInformation : public PerStateInformationBase {
private:
    const Entry default_value;
    typedef __gnu_cxx::hash_map<const StateRegistry *,
        SegmentedVector<Entry>, hash_pointer > EntryVectorMap;
    EntryVectorMap entries_by_registry;

    mutable const StateRegistry *cached_registry;
    mutable SegmentedVector<Entry> *cached_entries;

    SegmentedVector<Entry> *get_entries(const StateRegistry *registry) {
        if (cached_registry != registry) {
            cached_registry = registry;
            typename EntryVectorMap::value_type element(registry, SegmentedVector<Entry>());
            std::pair<typename EntryVectorMap::iterator, bool> inserted =
                entries_by_registry.insert(element);
            cached_entries = &inserted.first->second;
            if (inserted.second) {
                registry->subscribe(this);
            }
        }
        assert(cached_registry == registry && cached_entries == &entries_by_registry[registry]);
        return cached_entries;
    }

    const SegmentedVector<Entry> *get_entries(const StateRegistry *registry) const {
        if (cached_registry != registry) {
            typename EntryVectorMap::const_iterator it = entries_by_registry.find(registry);
            if (it != entries_by_registry.end()) {
                cached_registry = registry;
                cached_entries = const_cast<SegmentedVector<Entry> *>(&it->second);
            } else {
                return 0;
            }
        }
        assert(cached_registry == registry);
        return cached_entries;
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
        : PerStateInformationBase(),
          default_value(),
          cached_registry(0),
          cached_entries(0) {
    }

    explicit PerStateInformation(const Entry &default_value_)
        : PerStateInformationBase(),
          default_value(default_value_),
          cached_registry(0),
          cached_entries(0) {
    }

    ~PerStateInformation() {
        for (typename EntryVectorMap::iterator it = entries_by_registry.begin();
             it != entries_by_registry.end(); ++it) {
            it->first->unsubscribe(this);
        }
    }

    Entry &operator[](const State &state) {
        const StateRegistry *registry = state.get_registry();
        SegmentedVector<Entry> *entries = get_entries(registry);
        int state_id = state.get_id().value;
        size_t virtual_size = registry->size();
        assert(state_id >= 0 && state_id < virtual_size);
        if (entries->size() < virtual_size) {
            entries->resize(virtual_size, default_value);
        }
        return (*cached_entries)[state_id];
    }

    const Entry &operator[](const State &state) const {
        const StateRegistry *registry = state.get_registry();
        const SegmentedVector<Entry> *entries = get_entries(registry);
        if (!entries) {
            return default_value;
        }
        int state_id = state.get_id().value;
        assert(state_id >= 0 && state_id < registry->size());
        if (state_id >= entries->size()) {
            return default_value;
        }
        return (*entries)[state_id];
    }

    void state_registry_discarded(StateRegistry *registry) {
        entries_by_registry.erase(registry);
        if (registry == cached_registry) {
            cached_registry = 0;
            cached_entries = 0;
        }
    }
};

#endif
