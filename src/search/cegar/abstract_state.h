#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "domains.h"

#include <string>
#include <utility>
#include <vector>

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

    void add_arc(OperatorProxy op, AbstractState *other);

    void remove_arc(Arcs &arcs, OperatorProxy op, AbstractState *other);
    void remove_incoming_arc(OperatorProxy op, AbstractState *other);
    void remove_outgoing_arc(OperatorProxy op, AbstractState *other);

    void update_incoming_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_loops(int var, AbstractState *v1, AbstractState *v2);

    bool domains_intersect(const AbstractState *other, int var) const;

public:
    AbstractState(const Domains &domains, Node *node);
    ~AbstractState() = default;

    AbstractState(const AbstractState &) = delete;
    AbstractState &operator=(const AbstractState &) = delete;

    // Return the size of var's abstract domain for this state.
    size_t count(int var) const;

    // Return the set of states in which applying "op" leads to this state.
    Domains regress(OperatorProxy op) const;

    // Separate the values in "wanted" from the other values in the abstract domain.
    std::pair<AbstractState *, AbstractState *> split(int var, std::vector<int> wanted);

    void add_loop(OperatorProxy op);

    void get_possible_flaws(const AbstractState &desired,
                            const State &prev_conc_state,
                            Flaws *flaws) const;

    bool is_abstraction_of(const State &conc_state) const;
    bool is_abstraction_of(const AbstractState &abs_state) const;

    void set_h(int new_h);
    int get_h() const;

    const Arcs &get_outgoing_arcs() const {return outgoing_arcs; }
    const Arcs &get_incoming_arcs() const {return incoming_arcs; }
    const Loops &get_loops() const {return loops; }

    // Testing.
    std::string str() const;
};
}

#endif
