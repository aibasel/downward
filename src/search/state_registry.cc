#include "state_registry.h"

#include "axioms.h"
#include "operator.h"
#include "state_var_t.h"

using namespace std;

StateRegistry::StateRegistry()
    : state_data_pool(g_variable_domain.size()),
      registered_states(0,
                        StateIDSemanticHash(state_data_pool),
                        StateIDSemanticEqual(state_data_pool)),
      cached_initial_state(0) {
}


StateRegistry::~StateRegistry() {
    delete cached_initial_state;
}

StateID StateRegistry::insert_id_or_pop_state() {
    /*
      Attempt to insert a StateID for the last state of state_data_pool
      if none is present yet. If this fails (another entry for this state
      is present), we have to remove the duplicate entry from the
      state data pool.
    */
    StateID id(state_data_pool.size() - 1);
    pair<StateIDSet::iterator, bool> result = registered_states.insert(id);
    bool is_new_entry = result.second;
    if (!is_new_entry) {
        state_data_pool.pop_back();
    }
    assert(registered_states.size() == state_data_pool.size());
    return *result.first;
}

State StateRegistry::lookup_state(StateID id) const {
    return State(state_data_pool[id.value], id);
}

const State &StateRegistry::get_initial_state() {
    if (cached_initial_state == 0) {
        state_data_pool.push_back(g_initial_state_buffer);
        state_var_t *vars = state_data_pool[state_data_pool.size() - 1];
        g_axiom_evaluator->evaluate(vars);
        StateID id = insert_id_or_pop_state();
        cached_initial_state = new State(lookup_state(id));
    }
    return *cached_initial_state;
}

//TODO it would be nice to move the actual state creation (and operator application)
//     out of the StateRegistry. This could for example be done by global functions
//     operating on state buffers (state_var_t *).
State StateRegistry::get_successor_state(const State &predecessor, const Operator &op) {
    assert(!op.is_axiom());
    state_data_pool.push_back(predecessor.get_buffer());
    state_var_t *vars = state_data_pool[state_data_pool.size() - 1];
    for (size_t i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.does_fire(predecessor))
            vars[pre_post.var] = pre_post.post;
    }
    g_axiom_evaluator->evaluate(vars);
    StateID id = insert_id_or_pop_state();
    return lookup_state(id);
}
