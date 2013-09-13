#include "state_registry.h"

#include "state_var_t.h"

using namespace std;

StateRegistry::StateRegistry() {
    state_data_pool = new SegmentedArrayVector<state_var_t>(
        g_variable_domain.size());
}


StateRegistry::~StateRegistry() {
    delete state_data_pool;
}


StateHandle StateRegistry::get_handle(const State &state) {
    /*
      Attempt to insert a StateRepresentation for this state if none
      is present yet.

      If this succeeds (results in a new entry), we will then need to
      set the proper ID and copy the state data into it. Later, we
      should pack the data from state instead of copying it.

      If it does not succeed, return the existing entry.
    */

    pair<StateRepresentationSet::iterator, bool> result =
        registered_states.insert(StateRepresentation(state.get_buffer()));
    const StateRepresentation &registered_representation = *result.first;
    bool new_entry = result.second;
    if (new_entry) {
        // TODO: Code duplication with State::copy_buffer_from will
        // disappear when packed states are introduced. State will
        // then copy unpacked states while this method will copy the
        // packed representation.
        int id = state_data_pool->size();
        state_data_pool->push_back(registered_representation.data);
        assert(registered_states.size() == state_data_pool->size());

        registered_representation.id = id;
        // TODO: We don't really need a separate data field any more
        // because it follows from the id.
        // => additional space optimization opportunity
        registered_representation.data = (*state_data_pool)[id];
    }
    return StateHandle(&registered_representation);
}
