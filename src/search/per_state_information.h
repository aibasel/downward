#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "state.h"

#include <vector>
#include <assert.h>

template<class Entry>
class PerStateInformation {
private:
    Entry default_value;
    std::vector<Entry> entries;
public:
    PerStateInformation()
        : PerStateInformation(Entry()) {
    }
    PerStateInformation(Entry _default_value)
        : default_value(_default_value) {
    }

    Entry &operator[](const State &state) {
        assert(state.id >= 0);
        if (entries.size() <= state.id) {
            entries.resize(state.id + 1, default_value);
        }
        return entries[state.id];
    }
};

#endif
