#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "domains.h"

#include <string>
#include <utility>
#include <vector>

class ConditionsProxy;
class FactProxy;
class OperatorProxy;
class State;
class TaskProxy;

namespace CEGAR {
class AbstractState;
class Node;

using Arc = std::pair<OperatorProxy, AbstractState *>;
using Arcs = std::vector<Arc>;
using Loops = std::vector<OperatorProxy>;

class AbstractState {
private:
    // Abstract domains for all variables.
    const Domains domains;

    // This state's node in the refinement hierarchy.
    Node *node;

    // Transitions from and to other abstract states.
    Arcs incoming_arcs;
    Arcs outgoing_arcs;

    // Self-loops.
    Loops loops;

    // Construct instances with factory methods.
    AbstractState(const Domains &domains, Node *node);

    void add_arc(OperatorProxy op, AbstractState *other);

    void remove_arc(Arcs &arcs, OperatorProxy op, AbstractState *other);
    void remove_incoming_arc(OperatorProxy op, AbstractState *other);
    void remove_outgoing_arc(OperatorProxy op, AbstractState *other);

    void update_incoming_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_loops(int var, AbstractState *v1, AbstractState *v2);

    bool domains_intersect(const AbstractState *other, int var) const;

public:
    ~AbstractState() = default;

    AbstractState(const AbstractState &) = delete;
    AbstractState &operator=(const AbstractState &) = delete;

    AbstractState(AbstractState &&other);

    // Return the size of var's abstract domain for this state.
    size_t count(int var) const;

    bool contains(FactProxy fact) const;

    // Return the abstract state in which applying "op" leads to this state.
    AbstractState regress(OperatorProxy op) const;

    /*
      Split this state into two new states by separating the "wanted" values
      from the other values in the abstract domain and return the resulting two
      new states.
    */
    std::pair<AbstractState *, AbstractState *> split(
        int var, const std::vector<int> &wanted);

    void add_loop(OperatorProxy op);

    bool is_abstraction_of(const State &conc_state) const;
    bool is_abstraction_of(const AbstractState &abs_state) const;

    void set_h_value(int new_h);
    int get_h_value() const;

    const Arcs &get_outgoing_arcs() const {return outgoing_arcs; }
    const Arcs &get_incoming_arcs() const {return incoming_arcs; }
    const Loops &get_loops() const {return loops; }

    friend std::ostream &operator<<(std::ostream &os, const AbstractState &state) {
        return os << state.domains;
    }

    /*
      Create the initial unrefined abstract state on the heap. Must be deleted
      by the caller.
    */
    static AbstractState *get_trivial_abstract_state(
        TaskProxy task_proxy, Node *root_node);

    // Create the Cartesian set that corresponds to the given fact conditions.
    static AbstractState get_abstract_state(
        TaskProxy task_proxy, const ConditionsProxy &conditions);
};
}

#endif
