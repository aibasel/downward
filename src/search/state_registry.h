#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include "globals.h"
#include "segmented_vector.h"
#include "state.h"
#include "state_id.h"
#include "state_var_t.h"
#include "utilities.h"

#include <hash_set>

/*
  Overview of classes relevant to storing and working with registered states.

  State
    This class is used for manipulating states.
    It contains the (uncompressed) variable values for fast access by the heuristic.
    A State are always registered in a state registry and have a valid id.
    States can be constructed from a state registry by factory methods for the
    initial state and successor states.
    They never own the actual state data which is borrowed from the state registry
    that created them.

  StateID
    StateIDs identify states within a state registry.
    If the registry is known, the id is sufficient to look up the state, which
    is why ids are intended for long term storage (e.g. in open lists).
    They replace the previously used StateProxies and StateHandles.

  state_var_t*
    The actual state data is internaly represented as an array of type state_var_t.
    Currently there is no difference between compressed and uncompressed states.
    Arrays like this are created in state registries in-place in a SegmentedArrayVector.

  -------------

  StateRegistry
    The StateRegistry allows to create states giving them an id. It also
    stores the actual state data in a memory friendly way. It uses the following
    two classes:

  SegmentedArrayVector<state_var_t>
    This class is used to store the actual state data for all states
    while avoiding dynamically allocating each state individually.
    The index within this vector corresponds to the id of the state.

  StateIDSet
    Hash set of StateIDs used to detect states that are already registered in
    this registry and find their IDs. States are compared/hashed semantically,
    i.e. the actual state data is compared, not the memory location.

  -------------

  SearchNodeInfo
    Remaining part of a search node besides the state that needs to be stored.

  SearchNode
    A SearchNode combines a StateID, a reference to a SearchNodeInfo and
    OperatorCost. It is generated for easier access and not intended for long
    term storage.

  SearchSpace
    The SearchSpace maps StateIDs to SearchNodeInfos.

  -------------

  PerStateInformation
    Template class that stores one entry for each state.
    References to entries stay valid forever.

  SegmentedVector
    Container that stores elements in segments. All segments have the same length.
    Segments are never resized, so all references to entries stay valid.
*/

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
            return std::equal(lhs_data, lhs_data + size, rhs_data);
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
    // See issue386.
    StateID get_id(const State &state);
    State get_state(StateID id) const;

    State get_initial_state();
    State get_successor_state(const State &predecessor, const Operator &op);

    size_t size() const {
        return registered_states.size();
    }
};

#endif
