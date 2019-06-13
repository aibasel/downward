#ifndef STATE_HANDLE_H
#define STATE_HANDLE_H

#include "state_id.h"

#include <cassert>

class StateRegistry;

class StateHandle {
    const StateRegistry *registry;
    StateID id;
public:
    StateHandle(const StateRegistry *registry, StateID id)
        : registry(registry), id(id) {
        assert((registry && id != StateID::no_state) || (!registry && id == StateID::no_state));
    }

    StateHandle(const StateHandle &other) = default;
    StateHandle &operator=(const StateHandle &other) = default;

    const StateRegistry *get_registry() const {
        return registry;
    }

    const StateID get_id() const {
        return id;
    }

    static const StateHandle unregistered_state;

    bool operator==(const StateHandle &other) const {
        return registry == other.registry && id == other.id;
    }

    bool operator!=(const StateHandle &other) const {
        return !(*this == other);
    }
};


#endif
