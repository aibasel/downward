#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "globals.h"
#include "segmented_vector.h"
#include "state.h"
#include "state_id.h"
#include "state_registry.h"

#include <cassert>

template<class Entry>
class PerStateInformation {
private:
    StateRegistry &state_registry;
    const Entry default_value;
    SegmentedVector<Entry> entries;

public:
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

    Entry &operator[](const StateID &id) {
        int state_id = id.value;
        size_t virtual_size = state_registry.size();
        assert(state_id >= 0 && state_id < virtual_size);
        if (entries.size() < virtual_size) {
            entries.resize(virtual_size, default_value);
        }
        return entries[state_id];
    }

    const Entry &operator[](const StateID &id) const {
        // We do not change the size here to avoid having to make entries
        // mutable. Instead, we return the default value as a const refernce
        // if the index is out of bounds.
        int state_id = id.value;
        assert(state_id >= 0 && state_id < state_registry.size());
        if (state_id >= entries.size()) {
            return default_value;
        }
        return entries[state_id];
    }
};

#endif
