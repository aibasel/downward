#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include "globals.h"
#include "state.h"
#include "state_handle.h"
#include "state_var_t.h"
#include "utilities.h"

#include <hash_set>

template<typename T>
class SegmentedArrayVector;

/*
  Overview of classes relevant to storing and working with registered states.

  State
    This class is used for manipulating states.
    It contains the (uncompressed) variable values for fast access by the heuristic.
    A State can be registered or unregistered, i.e., its handle can be valid or not.
    States can be constructed by factory methods for the initial state and successor
    states or from a valid StateHandle.
    Constructed successor states own their data until they are registered. The
    ownership is then transfered to the StateRegistry. This means that copying an
    unregistered state is more expensive than copying a registered state.
    States are not intended for long term storage. Instead they should be registered
    and their handles should be stored.

  StateHandle
    StateHandles are light weight representativies of registered states. They can
    be obtained from a registered state or by registering a state in a registry.
    StateHandles are intended for long term storage (e.g. in open lists) and
    replace StateProxies.
    They do not own the state data of their respective state.

  StateRepresentation
    This is an internal class used only inside StateHandle and StateRegistry.
    StateRepresentations should not be used directly.
    They do not own the state data of their respective state.
    StateRepresentations are used for long term storage in the StateRegistry to
    associate an id with the (compressed) state data (state_var_t* for now).

  state_var_t*
    The actual state data is internaly represented as an array of type state_var_t.
    Currently there is no difference between compressed and uncompressed states.
    Arrays like this are created in the State class, and copied into a
    SegmentedArrayVector in the StateRegistry. The latter is used for long term
    storage. Registered states borrow the state_var_t* from the registry.

  -------------

  StateRegistry
    The StateRegistry allows to register states giving them a unique id. It also
    stores the actual state data in a memory friendly way.

  SegmentedArrayVector<state_var_t>
    This class is used to store the actual state data for all registered states
    while avoiding dynamically allocating each state individually.
    The index within this vector corresponds to the id of the state's handle.

  StateRepresentationSet
    Hash set of StateRepresentations used to detect states that are already
    registered and find their handles.

  -------------

  SearchNodeInfo
    Remaining part of a search node besides the state that needs to be stored.

  SearchNode
    A SearchNode combines a StateHandle, a reference to a SearchNodeInfo and
    OperatorCost. It is generated for easier access and not intended for long
    term storage.

  SearchSpace
    The SearchSpace maps StateHandles to SearchNodeInfos.

  -------------

  PerStateInformation
    Template class that stores one entry for each registered state.
    References to entries stay valid forever.

  SegmentedVector
    Container that stores elements in segments. All segments have the same length.
    Segments are never resized, so all references to entries stay valid.
*/

class StateRegistry {
    typedef StateHandle::StateRepresentation StateRepresentation;

    struct StateRepresentationSemanticHash {
      size_t operator() (const StateRepresentation &representation) const {
          return ::hash_number_sequence(representation.data, g_variable_domain.size());
      }
    };

    struct StateRepresentationSemanticEqual {
      size_t operator() (const StateRepresentation &lhs,
                         const StateRepresentation &rhs) const {
          size_t size = g_variable_domain.size();
          return ::equal(lhs.data, lhs.data + size, rhs.data);
      }
    };

    typedef __gnu_cxx::hash_set<StateRepresentation,
                                StateRepresentationSemanticHash,
                                StateRepresentationSemanticEqual> StateRepresentationSet;

    SegmentedArrayVector<state_var_t> *state_data_pool;
    StateRepresentationSet registered_states;
public:
    StateRegistry();
    ~StateRegistry();
    /*
       After calling this function the returned state is registered and has a
       valid handle.
       Performes a lookup of state. If the same state was previously looked up,
       a state with a valid handle to the registered data is returned.
       Otherwise the state is registered (i.e. gets an id and is stored for
       later lookups). After this registration a state with a valid handle to
       the state just registered is returned.
    */
    // TODO If State is split in RegieredState and UnregisteredState, change the
    // signature of this to
    // RegieredState get_registered_state(const UnregisteredState& unregistered_state);
    // See issue386.
    StateHandle get_handle(const State &state);

    size_t size() const {
        return registered_states.size();
    }

    /*
       An iterator would be nicer but we do not want to expose StateRepresentation.
       Since this method is only used by debugging output right now (i.e. in
       SearchSpace::dump()), the effort of writing a custom iterator that returns
       StateHandles instead of StateRepresentation* is not worth it.
    */
    std::vector<StateHandle> get_all_registered_handles() const {
        std::vector<StateHandle> handles;
        for (StateRepresentationSet::const_iterator it = registered_states.begin();
             it != registered_states.end(); ++it) {
            handles.push_back(StateHandle(&*it));
        }
        return handles;
    }
};

#endif
