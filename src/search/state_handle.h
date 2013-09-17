#ifndef STATE_HANDLE_H
#define STATE_HANDLE_H

#include "state_var_t.h"
#include <ext/hash_set>
#include <cassert>

class StateRegistry;
class State;

// PIMPL pattern (http://en.wikipedia.org/wiki/Opaque_pointer#C.2B.2B)
// StateRepresentation should not be used outside of this class and the StateRegistry.
// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.
class StateHandle {
    friend class StateRegistry;

    // Could be reduced to only allow the ctor, but this creates cyclic depencies
    // friend State::State(const StateHandle&);
    friend class State;
private:
    struct StateRepresentation {
        enum { INVALID_ID = -1 };
        explicit StateRepresentation(const state_var_t *_data)
            : id(INVALID_ID) {
            data = const_cast<state_var_t *>(_data);
        }
        ~StateRepresentation() {
        }

        // Fields are mutable so they can be changed after the object is inserted
        // into a hash_set, i.e. a state registry.
        mutable int id;
        // This will later be replaced by packed state data.
        mutable state_var_t *data;
    };

    const StateRepresentation* representation;
    // Private low level ctor and make_permanent only for use of state registry.
    explicit StateHandle(const StateRepresentation *representation_)
        : representation(representation_) {
        // If the handle is valid, it always contains data
        assert(!representation || representation->data);
    }

    // Buffer should not be accessed directly, but only through the State ctor
    const state_var_t *get_buffer() const {
        // This will later be replaced by a method unpacking the packed state
        // contained in representation.
        assert(representation);
        return representation->data;
    }
public:
    ~StateHandle() {
    }

    static StateHandle invalid;

    int get_id() const {
        assert(representation);
        return representation->id;
    }

    // TODO get rid of this if/when we split the State class into RegisteredState
    // and UnregisteredState. In this case UnregisteredState would not have a
    // handle and RegisteredState would always have a valid handle.
    // See issue386.
    bool is_valid() const {
        return representation != 0;
    }

    bool operator==(const StateHandle &other) const {
        return representation == other.representation;
    }

};

namespace __gnu_cxx {
template<>
struct hash<StateHandle> {
    size_t operator()(const StateHandle &handle) const {
        if (!handle.is_valid()) {
            // TODO Maybe use assert(handle.is_valid()) instead.
            // This would mean that invalid handles do not have a hash value, though.
            return 0;
        }
        return handle.get_id();
    }
};
}

#endif
