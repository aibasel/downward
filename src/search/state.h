#ifndef STATE_H
#define STATE_H

#include "globals.h"
#include "packed_state.h"
#include "state_id.h"

#include <iostream>
#include <vector>

class Operator;
class StateRegistry;

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.
class State {
    friend class StateRegistry;
    template <class Entry>
    friend class PerStateInformation;
    // Values for vars are maintained in a packed state and accessed on demand.
    ReadOnlyPackedState packed_state;
    // registry isn't a reference because we want to support operator=
    const StateRegistry *registry;
    StateID id;
    // Only used by the state registry.
    State(ReadOnlyPackedState &packed_state_, const StateRegistry &registry_,
          StateID id_);

    const ReadOnlyPackedState &get_packed_state() const {
        return packed_state;
    }

    const StateRegistry &get_registry() const {
        return *registry;
    }

    // No implementation to prevent default construction
    State();
public:
    ~State();

    StateID get_id() const {
        return id;
    }

    int operator[](int index) const{
        return packed_state.get(index);
    }

    void dump_pddl() const;
    void dump_fdr() const;
};

#endif
