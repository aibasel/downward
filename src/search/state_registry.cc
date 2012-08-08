#include "state_registry.h"

#include "state_var_t.h"

using namespace std;

StateRegistry::StateRegistry(): next_id(0) {
}

State StateRegistry::get_registered_state(const State &state) {
    // create a preliminary handle with a borrowed buffer and an invalid id
    StateHandle handle(state.get_buffer());
    pair<HandleSet::iterator, bool> result = registered_states.insert(handle);
    HandleSet::iterator it = result.first;
    bool new_entry = result.second;
    if (new_entry) {
        it->make_permanent(next_id++);
    }
    return State(*it);
}
