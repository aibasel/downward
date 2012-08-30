#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include <hash_set>
#include "state.h"
#include "state_handle.h"

class StateRegistry {
private:
    typedef __gnu_cxx::hash_set<StateHandle> HandleSet;
    HandleSet registered_states;
public:
    typedef HandleSet::const_iterator const_iterator;

    /* After calling this function the returned state is registered and has a
       valid handle.
       Performes a lookup of state. If the same state was previously looked up,
       a state with a valid handle to the registered data is returned.
       Otherwise the state is registered (i.e. gets an id and is stored for
       later lookups). After this registration a state with a valid handle to
       the state just registered is returned.
    */
    // TODO maybe clearer to return StateHandle instead. In this case call it
    // "get_state_handle" or "get_handle". Alternatively wait for the split in
    // RegieredState and UnregisteredState.
    State get_registered_state(const State &state);

    size_t size() const {
        return registered_states.size();
    }

    const_iterator begin() const {
        return registered_states.begin();
    }

    const_iterator end() const {
        return registered_states.end();
    }
};

#endif
