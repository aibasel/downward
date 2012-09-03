#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "state.h"
#include "state_handle.h"
#include "state_registry.h"
#include "globals.h"

#include <vector>
#include <cassert>

template<class Entry>
class PerStateInformation {
private:
    StateRegistry &state_registry;
    const Entry default_value;
    std::vector<Entry> entries;

public:
    PerStateInformation()
        : state_registry(g_state_registry),
          default_value() {
    }

    PerStateInformation(const Entry &default_value_)
        : state_registry(g_state_registry),
          default_value(default_value_) {
    }

    PerStateInformation(const Entry &default_value_, StateRegistry &state_registry_)
        : state_registry(state_registry_),
          default_value(default_value_) {
    }

    Entry &operator[](const StateHandle &state_handle) {
        int state_id = state_handle.get_id();
        size_t virtual_size = state_registry.size();
        assert(state_id >= 0 && state_id < virtual_size);
        while (entries.size() <= virtual_size) {
            // resize() and insert() do not guarantee amortized constant time.
            // In a lot of cases this will be called once for each state, so we
            // have to avoid copying the vector every time.
            entries.push_back(default_value);
        }
        return entries[state_id];
    }

    const Entry operator[](const StateHandle &state_handle) const {
        // We do not change the size here to avoid having to make entries
        // mutable. Instead, we return the default value by value if the index
        // is out of bounds.
        int state_id = state_handle.get_id();
        assert(state_id >= 0 && state_id < state_registry.size());
        if (state_id >= entries.size()) {
            return default_value;
        }
        return entries[state_id];
    }

    void clear() {
        entries.clear();
        entries.resize(state_registry.size(), default_value);
    }
};

#endif
