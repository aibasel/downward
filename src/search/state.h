#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <vector>
using namespace std;

class Operator;

#include "state_var_t.h"
#include "state_handle.h"

class State {
    // Values for vars. will later be converted to UnpackedStateData.
    state_var_t *vars;
    StateHandle handle;
    static state_var_t *copy_buffer_from(const state_var_t *buffer);
public:
    explicit State(state_var_t *buffer) : vars(buffer) {
    }
    explicit State(const State &predecessor, const Operator &op);
    explicit State(const StateHandle &handle);

    static State *create_initial_state(state_var_t *initial_state_vars);

    State(const State &state);
    State &operator=(const State &other);
    ~State();

    int get_id() const;

    int operator[](int index) const {
        return vars[index];
    }
    void dump() const;
    bool operator==(const State &other) const;
    bool operator<(const State &other) const;
    size_t hash() const;

    const state_var_t *get_buffer() const {
        return vars;
    }
};

#endif
