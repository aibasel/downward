#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "state.h"

#include <vector>
#include <assert.h>

template<class Entry>
class PerStateInformation {
private:
    const Entry default_value;
    std::vector<Entry> entries;
public:
    PerStateInformation()
        : default_value() {
    }
    PerStateInformation(const Entry &_default_value)
        : default_value(_default_value) {
    }

    Entry &operator[](int state_id) {
        return this->at(state_id);
    }

    Entry &at(int state_id) {
        assert(state_id >= 0);
        if (entries.size() <= state_id) {
            entries.resize(state_id + 1, default_value);
        }
        return entries[state_id];
    }

    Entry &operator[](const State &state) {
        return this->at(state.get_id());
    }

    int size() {
        return entries.size();
    }
};

#endif
