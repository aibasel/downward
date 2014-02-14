#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include "globals.h"
#include "segmented_vector.h"
#include "state.h"
#include "state_id.h"
#include "utilities.h"

#include <set>
#include <ext/hash_set>

/*
  Overview of classes relevant to storing and working with registered states.

  State
    This class is used for manipulating states.
    It contains the (uncompressed) variable values for fast access by the heuristic.
    A State is always registered in a StateRegistry and has a valid ID.
    States can be constructed from a StateRegistry by factory methods for the
    initial state and successor states.
    They never own the actual state data which is borrowed from the StateRegistry
    that created them.

  StateID
    StateIDs identify states within a state registry.
    If the registry is known, the ID is sufficient to look up the state, which
    is why ids are intended for long term storage (e.g. in open lists).
    Internally, a StateID is just an integer, so it is cheap to store and copy.

  PackedStateEntry* (usually the same as int*, depends on word size)
    The actual state data is internally represented as a PackedStateEntry array.
    Each PackedStateEntry can contain the values multiple variables.
    To minimize allocation overhead, the implementation stores the data of many
    such states in a single large array (see SegmentedArrayVector).
    PackedStateEntry arrays are never manipulated directly but through
    PackedState classes.

  ReadOnlyPackedState
    Wrapper for PackedStateEntry* that allows read access to the state.
  MutablePackedState
    Wrapper for PackedStateEntry* that allows read and write access to the state.

  -------------

  StateRegistry
    The StateRegistry allows to create states giving them an ID. IDs from
    different state registries must not be mixed.
    The StateRegistry also stores the actual state data in a memory friendly way.
    It uses the following class:

  SegmentedArrayVector<PackedStateEntry>
    This class is used to store the actual (packed) state data for all states
    while avoiding dynamically allocating each state individually.
    The index within this vector corresponds to the ID of the state.

  PerStateInformation<T>
    Associates a value of type T with every state in a given StateRegistry.
    Can be thought of as a very compactly implemented map from State to T.
    References stay valid forever. Memory usage is essentially the same as a
    vector<T> whose size is the number of states in the registry.


  ---------------
  Usage example 1
  ---------------
  Problem:
    A search node contains a state together with some information about how this
    state was reached and the status of the node. The state data is already
    stored and should not be duplicated. Open lists should in theory store search
    nodes but we want to keep the amount of data stored in the open list to a
    minimum.

  Solution:

    SearchNodeInfo
      Remaining part of a search node besides the state that needs to be stored.

    SearchNode
      A SearchNode combines a StateID, a reference to a SearchNodeInfo and
      OperatorCost. It is generated for easier access and not intended for long
      term storage. The state data is only stored once an can be accessed
      through the StateID.

    SearchSpace
      The SearchSpace uses PerStateInformation<SearchNodeInfo> to map StateIDs to
      SearchNodeInfos. The open lists only have to store StateIDs which can be
      used to look up a search node in the SearchSpace on demand.

  ---------------
  Usage example 2
  ---------------
  Problem:
    In the LMcount heuristic each state should store which landmarks are
    already reached when this state is reached. This should only require
    additional memory, when the LMcount heuristic is used.

  Solution:
    The heuristic object uses a field of type PerStateInformation<std::vector<bool> >
    to store for each state and each landmark whether it was reached in this state.
*/

class PerStateInformationBase;

class StateRegistry {
    struct StateIDSemanticHash {
        const SegmentedArrayVector<PackedStateEntry> &state_data_pool;
        StateIDSemanticHash(const SegmentedArrayVector<PackedStateEntry> &state_data_pool_)
            : state_data_pool (state_data_pool_) {
        }
        size_t operator() (StateID id) const {
            return ::hash_number_sequence(state_data_pool[id.value], g_state_size);
        }
    };

    struct StateIDSemanticEqual {
        const SegmentedArrayVector<PackedStateEntry> &state_data_pool;
        StateIDSemanticEqual(const SegmentedArrayVector<PackedStateEntry> &state_data_pool_)
            : state_data_pool (state_data_pool_) {
        }

        size_t operator() (StateID lhs, StateID rhs) const {
            size_t size = g_state_size;
            const PackedStateEntry *lhs_data = state_data_pool[lhs.value];
            const PackedStateEntry *rhs_data = state_data_pool[rhs.value];
            return std::equal(lhs_data, lhs_data + size, rhs_data);
        }
    };

    /*
      Hash set of StateIDs used to detect states that are already registered in
      this registry and find their IDs. States are compared/hashed semantically,
      i.e. the actual state data is compared, not the memory location.
    */
    typedef __gnu_cxx::hash_set<StateID,
                                StateIDSemanticHash,
                                StateIDSemanticEqual> StateIDSet;

    SegmentedArrayVector<PackedStateEntry> state_data_pool;
    StateIDSet registered_states;
    State *cached_initial_state;
    mutable std::set<PerStateInformationBase *> subscribers;
    StateID insert_id_or_pop_state();
public:
    StateRegistry();
    ~StateRegistry();

    /*
      Returns the state that was registered at the given ID. The ID must refer
      to a state in this registry. Do not mix IDs from from different registries.
    */
    State lookup_state(StateID id) const;

    /*
      Returns a reference to the initial state and registers it if this was not
      done before. The result is cached internally so subsequent calls are cheap.
    */
    const State &get_initial_state();

    /*
      Returns the state that results from applying op to predecessor and
      registers it if this was not done before. This is an expensive operation
      as it includes duplicate checking.
    */
    State get_successor_state(const State &predecessor, const Operator &op);

    /*
      Returns the number of states registered so far.
    */
    size_t size() const {
        return registered_states.size();
    }

    /*
      Remembers the given PerStateInformation. If this StateRegistry is
      destroyed, it notifies all subscribed PerStateInformation objects.
      The information stored in them that relates to states from this
      registry is then destroyed as well.
    */
    void subscribe(PerStateInformationBase *psi) const;
    void unsubscribe(PerStateInformationBase *psi) const;
};

#endif
