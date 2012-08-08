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

state_var_t *State::copy_buffer_from(const state_var_t *buffer) {
    state_var_t *copy = new state_var_t[g_variable_domain.size()];
    // Update values affected by operator.
    // TODO: Profile if memcpy could speed this up significantly,
    //       e.g. if we do blind A* search.
    for (int i = 0; i < g_variable_domain.size(); i++)
        copy[i] = buffer[i];
    return copy;
}

State::State(const State &predecessor, const Operator &op) {
    assert(!op.is_axiom());
    vars = copy_buffer_from(predecessor.get_buffer());
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.does_fire(predecessor))
            vars[pre_post.var] = pre_post.post;
    }
    g_axiom_evaluator->evaluate(vars);
}

State::State(const StateHandle &_handle) : handle(_handle) {
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
    state_var_t *copy = copy_buffer_from(initial_state_vars);
    g_axiom_evaluator->evaluate(copy);
    return new State(copy);
}

State::State(const State &other) : vars(other.vars), handle(other.handle) {
    // only states should be copied that are valid, otherwise the state buffer
    // can get invalid
    assert(handle.is_valid());
}

State &State::operator=(const State &other) {
    // only states should be copied that are valid, otherwise the state buffer
    // can get invalid
    assert(handle.is_valid());
    vars = other.vars;
    handle = other.handle;
    return *this;
}

State::~State() {
    // TODO this is the wrong condition
    if (!handle.is_valid()) {
        delete vars;
    }
}

int State::get_id() const {
    return handle.get_id();
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
