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
    const Entry default_value;
    // This is mutabale so it can automatically grow to hold one element for each
    // registered state.
    mutable std::vector<Entry> entries;

    void ensure_virtual_size() const {
        size_t virtual_size = g_state_registry.size();
        while (entries.size() <= virtual_size) {
            // resize() and insert() do not guarantee amortized constant time.
            // In a lot of cases this will be called once for each state, so we
            // have to avoid copying the vector every time.
            entries.push_back(default_value);
        }
    }

    Entry &at(int state_id) {
        ensure_virtual_size();
        assert(state_id >= 0 && state_id < entries.size());
        return entries[state_id];
    }

    const Entry &at(int state_id) const {
        ensure_virtual_size();
        assert(state_id >= 0 && state_id < entries.size());
        return entries[state_id];
    }

public:
    PerStateInformation()
        : default_value() {
    }
    PerStateInformation(const Entry &default_value_)
        : default_value(default_value_) {
    }

    // TODO get rid of this access style in favor of using StateHandle.
    Entry &operator[](const State &state) {
        return at(state.get_handle().get_id());
    }

    // TODO get rid of this access style in favor of using StateHandle.
    const Entry &operator[](const State &state) const {
        return at(state.get_handle().get_id());
    }

    Entry &operator[](const StateHandle &state_handle) {
        return at(state_handle.get_id());
    }

    const Entry &operator[](const StateHandle &state_handle) const {
        return at(state_handle.get_id());
    }

    // TODO get rid of this access style in favor of using StateHandle.
    Entry &operator[](int state_id) {
        return at(state_id);
    }

    // TODO get rid of this access style in favor of using StateHandle.
    const Entry &operator[](int state_id) const {
        return at(state_id);
    }

    void clear() {
        entries.clear();
        entries.resize(g_state_registry.size(), default_value);
    }
};

#endif
