#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <vector>
using namespace std;

class Operator;

#include "state_var_t.h"

class State {
    state_var_t *vars; // values for vars
    bool borrowed_buffer;
    void _allocate();
    void _deallocate();
    void _copy_buffer_from_state(const State &state);

    // unique id (is set lazily on the first lookup)
    // mutable to allow lazy assignment
    mutable int id;
    enum {UNKOWN_ID = -1};

    // private ctor for use of factory method only
    State(state_var_t *buffer, int _id);
public:
    // construct State objects for initial state and successors
    static State *create_initial_state(state_var_t *buffer);
    State(const State &predecessor, const Operator &op);
    // copy and assignment ctors maintain the state id
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

    explicit State(state_var_t *buffer) {
        id = UNKOWN_ID;
        vars = buffer;
        borrowed_buffer = true;
    }
    const state_var_t *get_buffer() const {
        return vars;
    }
};

#endif
