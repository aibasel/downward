#include "state_registry.h"

#include "state_var_t.h"

using namespace std;

StateRegistry::StateRegistry()
    : state_data_pool(new SegmentedArrayVector<state_var_t>(
                          g_variable_domain.size())),
      registered_states(0,
                        StateIDSemanticHash(*state_data_pool),
                        StateIDSemanticEqual(*state_data_pool)) {
}


StateRegistry::~StateRegistry() {
    delete state_data_pool;
}


StateID StateRegistry::get_id(const State &state) {
    /*
      Add the state to the state data pool and attempt to insert
      a StateID for it if none is present yet.

      If this fails (another entry for this state is present),
      we have to remove the duplicate entry from the state data pool.

      Later, we should pack the data from state instead of copying it.
    */
    StateID id(state_data_pool->size());
    state_data_pool->push_back(state.get_buffer());
    pair<StateIDSet::iterator, bool> result = registered_states.insert(id);
    bool new_entry = result.second;
    if (!new_entry) {
        state_data_pool->pop_back();
    }
    assert(registered_states.size() == state_data_pool->size());
    return *result.first;
}


State StateRegistry::get_registered_state(StateID id) const {
    return State((*state_data_pool)[id.value], id);
}
