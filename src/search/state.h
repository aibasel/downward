#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <vector>
using namespace std;

class Operator;
class StateRegistry;

#include "state_id.h"
#include "state_var_t.h"
#include "globals.h"

class State {
    friend class StateRegistry;
    // Values for vars. will later be converted to UnpackedStateData.
    state_var_t *vars;
    StateID id;
    // Only used by the state registry.
    explicit State(state_var_t *buffer, StateID id_);

    const state_var_t *get_buffer() const {
        return vars;
    }
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
    bool operator==(const State &other) const;
    bool operator<(const State &other) const;
    size_t hash() const;
};

#endif
