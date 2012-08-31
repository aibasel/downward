#include "state_registry.h"

#include "state_var_t.h"

using namespace std;

StateHandle StateRegistry::get_handle(const State &state) {
    // Create a preliminary StateReprentation with an invalid id.
    // Later we have to pack the data from state instead of borrowing it
    StateRepresentation *temp_representation = new StateRepresentation(state.get_buffer());
    pair<StateRepresentationSet::iterator, bool> result =
            registered_states.insert(temp_representation);
    StateRepresentation *registered_representation = *result.first;
    bool new_entry = result.second;
    if (new_entry) {
        registered_representation->id = registered_states.size() - 1;
        // Code duplication with State::copy_buffer_from will disappear when packed
        // states are introduced. State will then copy unpacked states while this
        // method will copy the packed representation.
        state_var_t *copy = new state_var_t[g_variable_domain.size()];
        // Update values affected by operator.
        // TODO: Profile if memcpy could speed this up significantly,
        //       e.g. if we do blind A* search.
        for (size_t i = 0; i < g_variable_domain.size(); ++i)
            copy[i] = registered_representation->data[i];
        registered_representation->data = copy;
    } else {
        delete temp_representation;
    }
    return StateHandle(registered_representation);
}
