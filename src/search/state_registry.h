#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include <hash_set>
#include "state.h"
#include "state_handle.h"

class StateRegistry {
private:
    typedef __gnu_cxx::hash_set<StateHandle> HandleSet;
    HandleSet registered_states;
    int next_id;
public:
    StateRegistry();
    // Lookup of state. If the same state was previously looked up, a state with
    // a valid handle to the registered data is returned. Otherwise the state is
    // registered (i.e. gets an id and is stored for later lookups). After this
    // registration s state with a valid handle to the state just registered is
    // returned.
    // In short: after calling this function the returned state is registered
    // and has a valid handle
    State get_registered_state(const State &state);
};

#endif
