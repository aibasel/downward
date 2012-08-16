#ifndef STATE_HANDLE_H
#define STATE_HANDLE_H

#include "state_var_t.h"
#include <ext/hash_set>

class StateRegistry;
class State;

// PIMPL pattern (http://en.wikipedia.org/wiki/Opaque_pointer#C.2B.2B)
// StateRepresentation should not be used outside of this class.
class StateHandle {
    friend class StateRegistry;
    friend class State;
    // Could be reduced to only allow the ctor, but this creates cyclic depencies
    // friend State::State(const StateHandle&);
private:
    enum { INVALID_HANDLE = -1 };
    struct StateRepresentation;
    StateRepresentation* representation;
    // Private low level ctor and make_permanent only for use of state registry.
    explicit StateHandle(const state_var_t *data);
    void make_permanent(int id) const;
    // Buffer should not be accessed directly, but only through the State ctor
    const state_var_t *get_buffer() const;
public:
    StateHandle();
    StateHandle(const StateHandle&);
    const StateHandle& operator=(StateHandle);
    ~StateHandle();

    int get_id() const;
    bool is_valid() const;

    bool operator==(const StateHandle &other) const;
    size_t hash() const;
};

namespace __gnu_cxx {
template<>
struct hash<StateHandle> {
    size_t operator()(const StateHandle &handle) const {
        return handle.hash();
    }
};
}

#endif
