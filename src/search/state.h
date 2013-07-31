#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <vector>
using namespace std;

class Operator;

#include "state_var_t.h"
#include "state_handle.h"
#include "globals.h"

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.
class State {
    bool borrowed_buffer;
    // Values for vars. will later be converted to UnpackedStateData.
    state_var_t *vars;
    StateHandle handle;
    void copy_buffer_from(const state_var_t *buffer);
    // Only used for creating the initial state.
    explicit State(state_var_t *buffer);
    explicit State(const State &predecessor, const Operator &op);
public:
    explicit State(const StateHandle &handle);
    State(const State &state);
    State &operator=(const State &other);
    ~State();

    // Creates an unregistered State object for the initial state on the heap.
    static State *create_initial_state(state_var_t *initial_state_vars);
    // Named constructor for registered States
    static State construct_registered_successor(const State &predecessor,
        const Operator &op, StateRegistry &state_registry = *g_state_registry);
    // Named constructor for unregistered States
    static State construct_unregistered_successor(const State &predecessor, const Operator &op);

    StateHandle get_handle() const {
        return handle;
    }

    int operator[](int index) const {
        return vars[index];
    }
    void dump_pddl() const;
    void dump_fdr() const;
    bool operator==(const State &other) const;
    bool operator<(const State &other) const;
    size_t hash() const;

    const state_var_t *get_buffer() const {
        return vars;
    }
};

#endif
