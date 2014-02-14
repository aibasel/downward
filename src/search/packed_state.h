#ifndef PACKED_STATE_H
#define PACKED_STATE_H

#include "state_var_t.h"

#include <cassert>


template<class PackedStateEntry>
class PackedState {
    friend class StateRegistry;
    PackedStateEntry *buffer;
    const PackedStateEntry *get_buffer() const {
        return buffer;
    }
public:
    PackedState(PackedStateEntry *buffer_);
    ~PackedState();

    int get(int index) const;
    void set(int index, int value);
};

typedef PackedState<state_var_t> MutablePackedState;
typedef PackedState<const state_var_t> ReadOnlyPackedState;


template<class PackedStateEntry>
PackedState<PackedStateEntry>::PackedState(PackedStateEntry *buffer_)
    : buffer(buffer_) {
    assert(buffer);
}

template<class PackedStateEntry>
PackedState<PackedStateEntry>::~PackedState() {
}

template<class PackedStateEntry>
int PackedState<PackedStateEntry>::get(int index) const {
    return buffer[index];
}

template<class PackedStateEntry>
void PackedState<PackedStateEntry>::set(int index, int value) {
    buffer[index] = value;
}


#endif
