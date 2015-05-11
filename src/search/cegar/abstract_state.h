#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "domains.h"

#include <string>
#include <utility>
#include <vector>

class ConditionsProxy;
class TaskProxy;

namespace cegar {
class AbstractState;
class Node;

using Arc = std::pair<OperatorProxy, AbstractState *>;
using Arcs = std::vector<Arc>;
using Loops = std::vector<OperatorProxy>;

class AbstractState {
private:
    // Abstract domains for each variable.
    const Domains domains;

    // This state's node in the refinement hierarchy.
    Node *node;

    // Transitions from and to other abstract states.
    Arcs incoming_arcs;
    Arcs outgoing_arcs;

    // Self-loops.
    Loops loops;

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

    AbstractState(AbstractState &&) = default;

    // Return the size of var's abstract domain for this state.
    size_t count(int var) const;

    bool contains(FactProxy fact) const;

    // Return the set of states in which applying "op" leads to this state.
    AbstractState regress(OperatorProxy op) const;

    // Separate the values in "wanted" from the other values in the abstract domain.
    std::pair<AbstractState *, AbstractState *> split(int var, std::vector<int> wanted);

    void add_loop(OperatorProxy op);

    Splits get_possible_splits(const AbstractState &desired,
                               const State &conc_state) const;

    bool is_abstraction_of(const State &conc_state) const;
    bool is_abstraction_of(const AbstractState &abs_state) const;

    void set_h(int new_h);
    int get_h() const;

    const Arcs &get_outgoing_arcs() const {return outgoing_arcs; }
    const Arcs &get_incoming_arcs() const {return incoming_arcs; }
    const Loops &get_loops() const {return loops; }

    friend std::ostream &operator<<(std::ostream &os, const AbstractState &state) {
        return os << state.domains;
    }

    // Create the initial unrefined abstract state on the heap. Must be deleted by the caller.
    static AbstractState *get_unrefined_abstract_state(TaskProxy task_proxy, Node *root_node);

    // Create the Cartesian set that corresponds to the given fact conditions.
    static AbstractState get_abstract_state(TaskProxy task_proxy, const ConditionsProxy &conditions);
};
}

#endif
