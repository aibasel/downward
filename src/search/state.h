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
    int id;
public:
    explicit State(istream &in);
    State(const State &state);
    State(const State &predecessor, const Operator &op);
    ~State();
    State &operator=(const State &other);

    int get_id();

    state_var_t &operator[](int index) {
        return vars[index];
    }
    int operator[](int index) const {
        return vars[index];
    }
    void dump() const;
    bool operator==(const State &other) const;
    bool operator<(const State &other) const;
    size_t hash() const;

    explicit State(state_var_t *buffer) {
        vars = buffer;
        borrowed_buffer = true;
    }
    const state_var_t *get_buffer() const {
        return vars;
    }
};

#endif
