#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "globals.h"
#include "segmented_vector.h"
#include "state.h"
#include "state_id.h"
#include "state_registry.h"

#include <cassert>
#include <iterator>

template<class Entry>
class PerStateInformation {
private:
    StateRegistry &state_registry;
    const Entry default_value;
    SegmentedVector<Entry> entries;

public:
    // TODO this iterates over StateIDs not over entries. Move it to StateRegistry?
    //      A better implementation would allow to iterate over pair<StateID, Entry>.
    class const_iterator : public std::iterator<std::forward_iterator_tag,
                                                StateID> {
        friend class PerStateInformation<Entry>;
        const PerStateInformation<Entry> &owner;
        StateID pos;

        const_iterator(const PerStateInformation<Entry> &owner_, size_t start)
            : owner(owner_), pos(start) {}
    public:
        const_iterator(const const_iterator& other)
            : owner(other.owner), pos(other.pos) {}

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
            return &owner == &rhs.owner && pos == rhs.pos;
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

    const_iterator begin() const {return const_iterator(*this, 0); }
    const_iterator end() const {return const_iterator(*this, entries.size()); }

    PerStateInformation()
        : state_registry(*g_state_registry),
          default_value() {
    }

    PerStateInformation(const Entry &default_value_)
        : state_registry(*g_state_registry),
          default_value(default_value_) {
    }

    PerStateInformation(const Entry &default_value_, StateRegistry &state_registry_)
        : state_registry(state_registry_),
          default_value(default_value_) {
    }

    Entry &operator[](StateID id) {
        int state_id = id.value;
        size_t virtual_size = state_registry.size();
        assert(state_id >= 0 && state_id < virtual_size);
        if (entries.size() < virtual_size) {
            entries.resize(virtual_size, default_value);
        }
        return entries[state_id];
    }

    const Entry &operator[](StateID id) const {
        int state_id = id.value;
        assert(state_id >= 0 && state_id < state_registry.size());
        if (state_id >= entries.size()) {
            return default_value;
        }
        return entries[state_id];
    }
};

#endif
