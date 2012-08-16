#include "state.h"

#include "axioms.h"
#include "globals.h"
#include "operator.h"
#include "utilities.h"
#include "state_registry.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;

void State::copy_buffer_from(const state_var_t *buffer) {
    vars = new state_var_t[g_variable_domain.size()];
    // Update values affected by operator.
    // TODO: Profile if memcpy could speed this up significantly,
    //       e.g. if we do blind A* search.
    for (int i = 0; i < g_variable_domain.size(); i++)
        vars[i] = buffer[i];
    borrowed_buffer = false;
}

State::State(state_var_t *buffer) : borrowed_buffer(true), vars(buffer) {
}

State::State(const State &predecessor, const Operator &op) {
    assert(!op.is_axiom());
    copy_buffer_from(predecessor.get_buffer());
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.does_fire(predecessor))
            vars[pre_post.var] = pre_post.post;
    }
    g_axiom_evaluator->evaluate(vars);
}

State::State(const StateHandle &_handle) : borrowed_buffer(true), handle(_handle) {
    if (handle.is_valid()) {
        // We can  treat the buffer as const because it is never changed in a
        // state object (only assignment can change it, but it changes the whole
        // state)
        // TODO think about the use of const here and in StateHandle

        // This assignment will later be changed into something like
        // vars = handle.unpack_state_data();
        vars = const_cast<state_var_t *>(handle.get_buffer());
    } else {
        vars = 0;
    }
}

State *State::create_initial_state(state_var_t *initial_state_vars) {
    g_axiom_evaluator->evaluate(initial_state_vars);
    State unregistered_state = State(initial_state_vars);
    const State &registered_state = g_state_registry.get_registered_state(unregistered_state);
    // make a copy on the heap
    return new State(registered_state);
}

State State::create_registered_successor(const State &predecessor, const Operator &op) {
    // TODO avoid extra copy of state here
    State unregistered_copy(predecessor, op);
    return g_state_registry.get_registered_state(unregistered_copy);
}

State State::create_unregistered_successor(const State &predecessor, const Operator &op) {
    return State(predecessor, op);
}


State::State(const State &other) :  borrowed_buffer(true), vars(other.vars), handle(other.handle) {
    // only states should be copied that are valid, otherwise the state buffer
    // can get invalid
    assert(handle.is_valid());
    // If a state without a borrowed buffer is copied, the buffer would have to
    // be copied as well, which we try to avoid.
    assert(other.borrowed_buffer);
}

State &State::operator=(const State &other) {
    if (this != &other) {
        // clean up the old value to avoid a memory leak
        if (!borrowed_buffer) {
            delete vars;
        }
        // only states should be copied that are valid, otherwise the state buffer
        // can get invalid
        assert(other.handle.is_valid());
        // If a state without a borrowed buffer is copied, the buffer would have to
        // be copied as well, which we try to avoid.
        assert(other.borrowed_buffer);
        borrowed_buffer = true;
        vars = other.vars;
        handle = other.handle;
    }
    return *this;
}

State::~State() {
    if (!borrowed_buffer) {
        delete vars;
    }
}

int State::get_id() const {
    return handle.get_id();
}

const StateHandle State::get_handle() const {
    return handle;
}

void State::dump() const {
    // We cast the values to int since we'd get bad output otherwise
    // if state_var_t == char.
    for (int i = 0; i < g_variable_domain.size(); i++)
        cout << "  " << g_variable_name[i] << ": "
             << static_cast<int>(vars[i]) << endl;
}

bool State::operator==(const State &other) const {
    if (handle.is_valid()) {
        return handle == other.handle;
    } else {
        int size = g_variable_domain.size();
        return ::equal(vars, vars + size, other.vars);
    }
}

bool State::operator<(const State &other) const {
    int size = g_variable_domain.size();
    return ::lexicographical_compare(vars, vars + size,
                                     other.vars, other.vars + size);
}

size_t State::hash() const {
    return ::hash_number_sequence(vars, g_variable_domain.size());
}
