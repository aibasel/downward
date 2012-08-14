#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "state.h"
#include "state_handle.h"
#include "state_registry.h"
#include "globals.h"

#include <vector>
#include <assert.h>

template<class Entry>
class PerStateInformation {
private:
    const Entry default_value;
    mutable std::vector<Entry> entries;

    void ensure_virtual_size() const {
        size_t virtual_size = g_state_registry.size();
        while (entries.size() <= virtual_size) {
            // resize and insert do not guarantee amortized constant time
            // in a lot of cases this will be called once for each state, so we
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
    PerStateInformation(const Entry &_default_value)
        : default_value(_default_value) {
    }

    Entry &operator[](const State &state) {
        return this->at(state.get_id());
    }

    const Entry &operator[](const State &state) const {
        return this->at(state.get_id());
    }

    Entry &operator[](const StateHandle &state_handle) {
        return this->at(state_handle.get_id());
    }

    const Entry &operator[](const StateHandle &state_handle) const {
        return this->at(state_handle.get_id());
    }

    Entry &operator[](int state_id) {
        return this->at(state_id);
    }

    const Entry &operator[](int state_id) const {
        return this->at(state_id);
    }
};

#endif
