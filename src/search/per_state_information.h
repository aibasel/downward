#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "state.h"

#include <vector>
#include <assert.h>

template<class Entry>
class PerStateInformation {
private:
    std::vector<bool> empty;
    std::vector<Entry> entries;
public:
    PerStateInformation() {}

    Entry &operator[](const State &state) {
        assert(state.id >= 0);
        if (entries.size() <= state.id) {
            entries.resize(state.id + 1);
            empty.resize(state.id + 1, true);
        }
        empty[state.id] = false;
        return entries[state.id];
    }

    bool has_entry(const State &state) const {
        return state.id >= 0 && state.id < entries.size() && !empty[state.id];
    }

    void remove_enty(const State &state) {
        if (!has_entry(state)) {
            return;
        }
        empty[state.id] = true;
        entries[state.id] = Entry();
    }
};

#endif
