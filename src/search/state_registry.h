#ifndef STATE_REGISTRY_H
#define STATE_REGISTRY_H

#include "globals.h"
#include "segmented_vector.h"
#include "state.h"
#include "state_handle.h"
#include "state_var_t.h"
#include "utilities.h"

#include <hash_set>


class StateRegistry {
    typedef StateHandle::StateRepresentation StateRepresentation;

    struct StateRepresentationSemanticHash {
        size_t operator()(const StateRepresentation &representation) const {
            return ::hash_number_sequence(representation.data, g_variable_domain.size());
        }
    };

    struct StateRepresentationSemanticEqual {
        size_t operator()(const StateRepresentation &lhs,
                          const StateRepresentation &rhs) const {
            size_t size = g_variable_domain.size();
            return std::equal(lhs.data, lhs.data + size, rhs.data);
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

    /* After calling this function the returned state is registered and has a
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
    StateHandle get_handle(const State &state);

    size_t size() const {
        return registered_states.size();
    }

    /* An iterator would be nicer but we do not want to expose StateRepresentation.
       Since this method is only used by debugging output right now (i.e. in
       SearchSpace::dump()), the effort of writing a custom iterator that returns
       StateHandles instead of StateRepresentation* is not worth it. */
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
