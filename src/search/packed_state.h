#ifndef PACKED_STATE_H
#define PACKED_STATE_H

#include <cassert>
#include <vector>


class PackedStateMasks {
public:
    static int set_variables(const std::vector<int> &variable_domain) {
        // TODO use less than one PackedStateEntry per variable
        return variable_domain.size();
    }
};

// TODO Make this dependent on the size of a word (32/64 bit).
typedef int PackedStateEntry;

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
