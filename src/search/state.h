#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <vector>

class Operator;
class StateRegistry;

#include "state_id.h"
#include "state_var_t.h"
#include "globals.h"

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.
class State {
    friend class StateRegistry;
    template <class Entry>
    friend class PerStateInformation;
    // Values for vars. will later be converted to UnpackedStateData.
    const state_var_t *vars;
    const StateRegistry *registry;
    StateID id;
    // Only used by the state registry.
    State(const state_var_t *buffer, const StateRegistry *registry_,
          StateID id_);

    const state_var_t *get_buffer() const {
        return vars;
    }

    const StateRegistry *get_registry() const {
        return registry;
    }

    // No implementation to prevent default construction
    State();
public:
    ~State();

    StateID get_id() const {
        return id;
    }

    int operator[](int index) const {
        return vars[index];
    }
    void dump_pddl() const;
    void dump_fdr() const;
};

#endif
