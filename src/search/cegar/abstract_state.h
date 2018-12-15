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
    // Abstract domains for all variables.
    const Domains domains;

    // This state's node in the refinement hierarchy.
    Node *node;

    // Construct instances with factory methods.
    AbstractState(const Domains &domains, Node *node);

    bool is_more_general_than(const AbstractState &other) const;

public:
    AbstractState(const AbstractState &) = delete;

    AbstractState(AbstractState &&other);

    bool domains_intersect(const AbstractState *other, int var) const;

    // Return the size of var's abstract domain for this state.
    int count(int var) const;

    bool contains(int var, int value) const;

    // Return the abstract state in which applying "op" leads to this state.
    AbstractState regress(const OperatorProxy &op) const;

    /*
      Split this state into two new states by separating the "wanted" values
      from the other values in the abstract domain and return the resulting two
      new states.
    */
    std::pair<AbstractState *, AbstractState *> split(
        int var, const std::vector<int> &wanted, int v1_id, int v2_id);

    bool includes(const State &concrete_state) const;
    bool includes(const std::vector<FactPair> &facts) const;

    // IDs are consecutive, so they can be used to index states in vectors.
    int get_id() const;

    friend std::ostream &operator<<(std::ostream &os, const AbstractState &state) {
        return os << "#" << state.get_id() << state.domains;
    }

    /*
      Create the initial unrefined abstract state on the heap. Must be deleted
      by the caller.

      TODO: Return unique_ptr?
    */
    static AbstractState *get_trivial_abstract_state(
        const std::vector<int> &domain_sizes, Node *root_node);

    // Create the Cartesian set that corresponds to the given fact conditions.
    static AbstractState get_cartesian_set(
        const std::vector<int> &domain_sizes, const ConditionsProxy &conditions);
};
}

#endif
