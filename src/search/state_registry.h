#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include "globals.h"
#include "segmented_vector.h"
#include "state.h"
#include "state_id.h"
#include "state_var_t.h"
#include "utilities.h"

#include <hash_set>


class StateRegistry {
    struct StateIDSemanticHash {
        const SegmentedArrayVector<state_var_t> &state_data_pool;
        StateIDSemanticHash(const SegmentedArrayVector<state_var_t> &state_data_pool_)
            : state_data_pool (state_data_pool_) {
        }
        size_t operator() (const StateID &id) const {
            return ::hash_number_sequence(state_data_pool[id.value], g_variable_domain.size());
        }
    };

    struct StateIDSemanticEqual {
        const SegmentedArrayVector<state_var_t> &state_data_pool;
        StateIDSemanticEqual(const SegmentedArrayVector<state_var_t> &state_data_pool_)
            : state_data_pool (state_data_pool_) {
        }

        size_t operator() (const StateID &lhs,
                           const StateID &rhs) const {
            size_t size = g_variable_domain.size();
            const state_var_t *lhs_data = state_data_pool[lhs.value];
            const state_var_t *rhs_data = state_data_pool[rhs.value];
            return ::equal(lhs_data, lhs_data + size, rhs_data);
        }
    };

    typedef __gnu_cxx::hash_set<StateID,
                                StateIDSemanticHash,
                                StateIDSemanticEqual> StateIDSet;

    // TODO: why do we need a pointer here?
    SegmentedArrayVector<state_var_t> *state_data_pool;
    StateIDSet registered_states;
    State *cached_initial_state;
    StateID insert_id_or_pop_state();
public:
    StateRegistry();
    ~StateRegistry();

    /*
       After calling this function the returned state is registered and has a
       valid id.
       Performes a lookup of state. If the same state was previously looked up,
       a state with a valid id of the registered data is returned.
       Otherwise the state is registered (i.e. gets an id and is stored for
       later lookups). After this registration a state with a valid id is
       returned.
    */
    // TODO If State is split in RegieredState and UnregisteredState, change the
    // signature of this to
    // RegieredState get_registered_state(const UnregisteredState& unregistered_state);
    StateID get_id(const State &state);
    State get_state(StateID id) const;

    State get_initial_state();
    State get_successor_state(const State &predecessor, const Operator &op);

    size_t size() const {
        return registered_states.size();
    }
};

#endif
