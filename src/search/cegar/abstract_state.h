#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "domains.h"
#include "types.h"

#include <vector>

class ConditionsProxy;
struct FactPair;
class OperatorProxy;
class State;
class TaskProxy;

namespace cegar {
class Node;

/*
  Store and update abstract Domains.
*/
class AbstractState {
    int state_id;

    // This state's node in the refinement hierarchy.
    NodeID node_id;

    // Abstract domains for all variables.
    Domains domains;

public:
    AbstractState(int state_id, NodeID node_id, const Domains &domains);

    AbstractState(const AbstractState &) = delete;

    // TODO: Remove this method once we use unique_ptr for AbstractState.
    AbstractState(AbstractState &&other);

    bool domains_intersect(const AbstractState *other, int var) const;

    // Return the size of var's abstract domain for this state.
    int count(int var) const;

    bool contains(int var, int value) const;

    // Return the abstract state in which applying "op" leads to this state.
    AbstractState regress(const OperatorProxy &op) const;

    /*
      Separate the "wanted" values from the other values in the abstract domain
      and return the resulting two new Cartesian sets.
    */
    std::pair<Domains, Domains> split_domain(int var, const std::vector<int> &wanted);

    bool includes(const AbstractState &other) const;
    bool includes(const State &concrete_state) const;
    bool includes(const std::vector<FactPair> &facts) const;

    // IDs are consecutive, so they can be used to index states in vectors.
    int get_id() const;

    NodeID get_node_id() const;

    friend std::ostream &operator<<(std::ostream &os, const AbstractState &state) {
        return os << "#" << state.get_id() << state.domains;
    }

    /*
      Create the initial unrefined abstract state on the heap. Must be deleted
      by the caller.

      TODO: Return unique_ptr?
    */
    static AbstractState *get_trivial_abstract_state(
        const std::vector<int> &domain_sizes);

    // Create the Cartesian set that corresponds to the given fact conditions.
    static AbstractState get_cartesian_set(
        const std::vector<int> &domain_sizes, const ConditionsProxy &conditions);
};
}

#endif
