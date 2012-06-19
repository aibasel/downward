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

public:
    explicit State(istream &in);
    State(const State &state);
    State(const State &predecessor, const Operator &op);
    ~State();
    State &operator=(const State &other);

    // unique id (is set after a node lookup because we do not know
    // if this state is a duplicate before that)
    int id;

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

    explicit State(state_var_t *buffer, int id=0) {
        vars = buffer;
        borrowed_buffer = true;
        this->id = id;
    }
    const state_var_t *get_buffer() const {
        return vars;
    }
};

#endif
