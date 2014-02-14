#ifndef PACKED_STATE_H
#define PACKED_STATE_H

#include "globals.h"
#include "packed_state_entry.h"

#include <cassert>
#include <vector>


struct PackedStateProperties {
    int state_size;
    int *shift;
    int *entry;
    int *read_mask;
    int *clear_mask;
    PackedStateProperties(const std::vector<int> &variable_domain);
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
    return buffer[index];
}

template<class Entry>
void PackedState<Entry>::set(int index, int value) {
    buffer[index] = value;
}


#endif
