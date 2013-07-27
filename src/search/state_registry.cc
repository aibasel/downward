#include "state_registry.h"

#include "state_var_t.h"

using namespace std;

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
        registered_representation.id = registered_states.size() - 1;
        // Code duplication with State::copy_buffer_from will disappear when packed
        // states are introduced. State will then copy unpacked states while this
        // method will copy the packed representation.
        state_var_t *copy = new state_var_t[g_variable_domain.size()];
        // Update values affected by operator.
        // TODO: Profile if memcpy could speed this up significantly,
        //       e.g. if we do blind A* search.
        for (size_t i = 0; i < g_variable_domain.size(); ++i)
            copy[i] = registered_representation.data[i];
        registered_representation.data = copy;
    }
    return StateHandle(&registered_representation);
}
