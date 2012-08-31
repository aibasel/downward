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
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        vars[i] = buffer[i];
    borrowed_buffer = false;
}

State::State(state_var_t *buffer)
    : borrowed_buffer(true), vars(buffer), handle(StateHandle::invalid) {
    assert(vars);
}

State::State(const State &predecessor, const Operator &op)
    : handle(StateHandle::invalid) {
    assert(!op.is_axiom());
    copy_buffer_from(predecessor.get_buffer());
    for (size_t i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.does_fire(predecessor))
            vars[pre_post.var] = pre_post.post;
    }
    g_axiom_evaluator->evaluate(vars);
}

State::State(const StateHandle &handle_)
    : borrowed_buffer(true), handle(handle_) {
    if (!handle.is_valid()) {
        cout << "Tried to create State from invalid StateHandle." << endl;
        abort();
    }
    // We can  treat the buffer as const because it is never changed in a
    // state object (only assignment can change it, but it changes the whole
    // state).
    // TODO think about the use of const here and in StateHandle.

    // This assignment will later be changed into something like this:
    // vars = handle.unpack_state_data();
    // The buffer will no longer be shared then
    vars = const_cast<state_var_t *>(handle.get_buffer());
    assert(vars);
}

State *State::create_initial_state(state_var_t *initial_state_vars) {
    assert(g_axiom_evaluator);
    g_axiom_evaluator->evaluate(initial_state_vars);
    return new State(initial_state_vars);
}

State State::construct_registered_successor(const State &predecessor, const Operator &op) {
    State unregistered_copy(predecessor, op);
    return State(g_state_registry.get_handle(unregistered_copy));
}

State State::construct_unregistered_successor(const State &predecessor, const Operator &op) {
    return State(predecessor, op);
}

State::State(const State &other)
    : borrowed_buffer(true), vars(other.vars), handle(other.handle) {
    // If a state without a borrowed buffer is copied, the buffer would have to
    // be copied as well, which we try to avoid.
    // TODO this assert doesn't hold for unregistered states. Those should be
    // deep copied.
    assert(other.borrowed_buffer);
    // Every State needs valid state variables.
    assert(vars);
    // Only states should be copied that are valid, otherwise the state buffer
    // can get invalid.
    // TODO same as above: doesn't hold for unregistered states.
    assert(handle.is_valid());
}

State &State::operator=(const State &other) {
    if (this != &other) {
        // Clean up the old value to avoid a memory leak.
        if (!borrowed_buffer) {
            delete vars;
        }
        // Only states should be copied that are valid, otherwise the state buffer
        // can get invalid.
        // TODO Not valid: see above.
        assert(other.handle.is_valid());
        // If a state without a borrowed buffer is copied, the buffer would have to
        // be copied as well, which we try to avoid.
        // TODO Not valid: see above.
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

void State::dump() const {
    // We cast the values to int since we'd get bad output otherwise
    // if state_var_t == char.
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        cout << "  " << g_variable_name[i] << ": "
             << static_cast<int>(vars[i]) << endl;
}

bool State::operator==(const State &other) const {
    if (handle.is_valid()) {
        return handle == other.handle;
    } else if (vars == other.vars) {
        // borrowed from same buffer
        return true;
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
