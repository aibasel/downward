#ifndef PACKED_STATE_H
#define PACKED_STATE_H

#include "globals.h"
#include "packed_state_entry.h"

#include <cassert>
#include <vector>

struct PackedVariable {
    int index;
    int shift;
    PackedStateEntry read_mask;
    PackedStateEntry clear_mask;
};

struct PackedStateProperties {
    int state_size;
    PackedVariable *variables;
    PackedStateProperties(const std::vector<int> &variable_domain);
    ~PackedStateProperties();
};

template<class Entry>
class PackedState {
    friend class StateRegistry;
    Entry *buffer;
    const Entry *get_buffer() const {
        return buffer;
    }
public:
    PackedState(Entry *buffer_);
    ~PackedState();

    int get(int index) const;
    void set(int index, int value);
};

typedef PackedState<PackedStateEntry> MutablePackedState;
typedef PackedState<const PackedStateEntry> ReadOnlyPackedState;


template<class Entry>
PackedState<Entry>::PackedState(Entry *buffer_)
    : buffer(buffer_) {
    assert(g_packed_state_properties);
    assert(buffer);
}

template<class Entry>
PackedState<Entry>::~PackedState() {
}

template<class Entry>
int PackedState<Entry>::get(int index) const {
    const PackedVariable &var = g_packed_state_properties->variables[index];
    return (buffer[var.index] & var.read_mask) >> var.shift;
}

template<class Entry>
void PackedState<Entry>::set(int index, int value) {
    const PackedVariable &var = g_packed_state_properties->variables[index];
    PackedStateEntry before = buffer[var.index];
    buffer[var.index] = (before & var.clear_mask) | (value << var.shift);
}


#endif
