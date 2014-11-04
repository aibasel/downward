#include "state_registry.h"

#include "axioms.h"
#include "global_operator.h"
#include "per_state_information.h"

using namespace std;

StateRegistry::StateRegistry()
    : state_data_pool(g_state_packer->get_num_bins()),
      registered_states(0,
                        StateIDSemanticHash(state_data_pool),
                        StateIDSemanticEqual(state_data_pool)),
      cached_initial_state(0) {
}


StateRegistry::~StateRegistry() {
    for (set<PerStateInformationBase *>::iterator it = subscribers.begin();
         it != subscribers.end(); ++it) {
        (*it)->remove_state_registry(this);
    }
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

GlobalState StateRegistry::lookup_state(StateID id) const {
    return GlobalState(state_data_pool[id.value], *this, id);
}

const GlobalState &StateRegistry::get_initial_state() {
    if (cached_initial_state == 0) {
        PackedStateBin *buffer = new PackedStateBin[g_state_packer->get_num_bins()];
        // Avoid garbage values in half-full bins.
        fill_n(buffer, g_state_packer->get_num_bins(), 0);
        for (size_t i = 0; i < g_initial_state_data.size(); ++i) {
            g_state_packer->set(buffer, i, g_initial_state_data[i]);
        }
        g_axiom_evaluator->evaluate(buffer);
        state_data_pool.push_back(buffer);
        // buffer is copied by push_back
        delete[] buffer;
        StateID id = insert_id_or_pop_state();
        cached_initial_state = new GlobalState(lookup_state(id));
    }
    return *cached_initial_state;
}

//TODO it would be nice to move the actual state creation (and operator application)
//     out of the StateRegistry. This could for example be done by global functions
//     operating on state buffers (PackedStateBin *).
GlobalState StateRegistry::get_successor_state(const GlobalState &predecessor, const GlobalOperator &op) {
    assert(!op.is_axiom());
    state_data_pool.push_back(predecessor.get_packed_buffer());
    PackedStateBin *buffer = state_data_pool[state_data_pool.size() - 1];
    for (size_t i = 0; i < op.get_effects().size(); ++i) {
        const GlobalEffect &effect = op.get_effects()[i];
        if (effect.does_fire(predecessor))
            g_state_packer->set(buffer, effect.var, effect.val);
    }
    g_axiom_evaluator->evaluate(buffer);
    StateID id = insert_id_or_pop_state();
    return lookup_state(id);
}

void StateRegistry::subscribe(PerStateInformationBase *psi) const {
    subscribers.insert(psi);
}

void StateRegistry::unsubscribe(PerStateInformationBase *const psi) const {
    subscribers.erase(psi);
}
