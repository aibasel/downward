#ifndef STATE_ID_H
#define STATE_ID_H

#include <hash_set>

struct StateID {
    int value;
    explicit StateID(int value_)
        : value(value_) {
    }

    ~StateID() {
    }

    static StateID invalid;

    bool is_valid() const {
        return value >= 0;
    }
};

namespace __gnu_cxx {
template<>
struct hash<StateID> {
    size_t operator()(const StateID &id) const {
        return id.value;
    }
};
}

#endif
