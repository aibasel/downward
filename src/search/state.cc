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
    : borrowed_buffer(true), vars(buffer), id(StateID::invalid) {
    assert(vars);
}

State::State(const State &predecessor, const Operator &op)
    : id(StateID::invalid) {
    assert(!op.is_axiom());
    copy_buffer_from(predecessor.get_buffer());
    for (size_t i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.does_fire(predecessor))
            vars[pre_post.var] = pre_post.post;
    }
    g_axiom_evaluator->evaluate(vars);
}

State::State(state_var_t *buffer, StateID id_)
    : borrowed_buffer(true),
      vars(buffer),
      id(id_) {
    // The assignment vars(buffer) will later be changed into something like this:
    // vars = unpack_state_data(buffer);
    // The buffer will no longer be shared then
    if (!id.is_valid()) {
        cerr << "Tried to create State from invalid StateID." << endl;
        exit_with(EXIT_CRITICAL_ERROR);
    }
    assert(vars);
}

State *State::create_initial_state(state_var_t *initial_state_vars) {
    assert(g_axiom_evaluator);
    g_axiom_evaluator->evaluate(initial_state_vars);
    return new State(initial_state_vars);
}

State State::construct_registered_successor(const State &predecessor,
                                            const Operator &op,
                                            StateRegistry &state_registry) {
    State unregistered_copy(predecessor, op);
    return state_registry.get_registered_state(unregistered_copy);
}

State State::construct_unregistered_successor(const State &predecessor, const Operator &op) {
    return State(predecessor, op);
}

State::State(const State &other)
    : borrowed_buffer(other.borrowed_buffer),
      vars(other.vars),
      id(other.id) {
    // Every State needs valid state variables.
    assert(vars);
    // A registered state should always have a borrowed buffer.
    assert(!id.is_valid() || borrowed_buffer);

    if (!borrowed_buffer) {
        // Deep copy for unregistered states.
        copy_buffer_from(other.vars);
    }
}

State &State::operator=(const State &other) {
    if (this != &other) {
        // Clean up the old value to avoid a memory leak.
        if (!borrowed_buffer) {
            delete[] vars;
        }

        borrowed_buffer = other.borrowed_buffer;
        vars = other.vars;
        id = other.id;

        // Every State needs valid state variables.
        assert(vars);
        // A registered state should always have a borrowed buffer.
        assert(!id.is_valid() || borrowed_buffer);

        if (!borrowed_buffer) {
            // Deep copy for unregistered states.
            copy_buffer_from(other.vars);
        }
    }
    return *this;
}

State::~State() {
    if (!borrowed_buffer) {
        delete[] vars;
    }
}

void State::dump_pddl() const {
    for (int i = 0; i < g_variable_domain.size(); i++) {
        const string &fact_name = g_fact_names[i][vars[i]];
        if (fact_name != "<none of those>")
            cout << fact_name << endl;
    }
}

void State::dump_fdr() const {
    // We cast the values to int since we'd get bad output otherwise
    // if state_var_t == char.
    for (size_t i = 0; i < g_variable_domain.size(); ++i)
        cout << "  #" << i << " [" << g_variable_name[i] << "] -> "
             << static_cast<int>(vars[i]) << endl;
}

bool State::operator==(const State &other) const {
    if (id.is_valid()) {
        return id.value == other.id.value;
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
