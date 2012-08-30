#include "state_registry.h"

#include "state_var_t.h"

using namespace std;

State StateRegistry::get_registered_state(const State &state) {
    // create a preliminary handle with a borrowed buffer and an invalid id
    StateHandle handle(state.get_buffer());
    pair<HandleSet::iterator, bool> result = registered_states.insert(handle);
    HandleSet::iterator it = result.first;
    bool new_entry = result.second;
    if (new_entry) {
        it->make_permanent(registered_states.size() - 1);
    }
    return State(*it);
}
