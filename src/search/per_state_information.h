#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "state.h"

#include <vector>
#include <assert.h>

template<class Entry>
class PerStateInformationByValue {
private:
    Entry default_value;
    std::vector<Entry> entries;
public:
    PerStateInformationByValue(Entry _default_value):
        default_value(_default_value) {
    }

    Entry &operator[](const State &state) {
        assert(state.id >= 0);
        if (entries.size() <= state.id) {
            entries.resize(state.id + 1, default_value);
        }
        return entries[state.id];
    }
};


template<class Entry>
class PerStateInformationByReference {
private:
    std::vector<Entry *> entries;
public:
    PerStateInformationByReference() {}

    void set(const State &state, Entry &entry) {
        assert(state.id >= 0);
        if (entries.size() <= state.id) {
            entries.resize(state.id + 1, 0);
        }
        entries[state.id] = new Entry(entry);
    }

    Entry *get(const State &state) {
        assert(state.id >= 0);
        if (has_entry(state)) {
            return entries[state.id];
        } else {
            return 0;
        }
    }

    bool has_entry(const State &state) const {
        return state.id >= 0 && state.id < entries.size() && entries[state.id] != 0;
    }

    void remove_enty(const State &state) {
        if (!has_entry(state)) {
            return;
        }
        delete entries[state.id];
        entries[state.id] = 0;
    }
};

#endif
