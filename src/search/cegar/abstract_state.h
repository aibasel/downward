#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "arc.h"
#include "domains.h"

#include "../task_proxy.h"

#include <string>
#include <utility>
#include <vector>

namespace cegar {
class AbstractState;
class Node;

using Arcs = std::vector<Arc>;

// To save space we store self-loops (operator indices) separately.
using Loops = std::vector<int>;

class AbstractSearchInfo {
    int g;
    Arc incoming_arc;

    static const int UNDEFINED_OPERATOR;

public:
    AbstractSearchInfo()
        : incoming_arc(UNDEFINED_OPERATOR, nullptr) {
        reset();
    }

    ~AbstractSearchInfo() = default;

    void reset() {
        g = std::numeric_limits<int>::max();
        incoming_arc = Arc(UNDEFINED_OPERATOR, nullptr);
    }

    void decrease_g_value_to(int new_g) {
        assert(new_g <= g);
        g = new_g;
    }

    int get_g_value() const {
        return g;
    }

    void set_incoming_arc(const Arc &arc) {
        incoming_arc = arc;
    }

    const Arc &get_incoming_arc() const {
        assert(incoming_arc.op_id != UNDEFINED_OPERATOR && incoming_arc.target);
        return incoming_arc;
    }
};

class AbstractState {
private:
    // Since the abstraction owns the state we don't need AbstractTask.
    const TaskProxy &task_proxy;

    // Abstract domains for all variables.
    const Domains domains;

    // This state's node in the refinement hierarchy.
    Node *node;

    // Transitions from and to other abstract states.
    Arcs incoming_arcs;
    Arcs outgoing_arcs;

    // Self-loops.
    Loops loops;

    AbstractSearchInfo search_info;

    // Construct instances with factory methods.
    AbstractState(
        const TaskProxy &task_proxy, const Domains &domains, Node *node);

    void add_arc(int op_id, AbstractState *other);
    void add_loop(int op_id);

    void remove_arc(Arcs &arcs, int op_id, AbstractState *other);
    void remove_incoming_arc(int op_id, AbstractState *other);
    void remove_outgoing_arc(int op_id, AbstractState *other);

    void split_incoming_arcs(int var, AbstractState *v1, AbstractState *v2);
    void split_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2);
    void split_loops(int var, AbstractState *v1, AbstractState *v2);

    bool domains_intersect(const AbstractState *other, int var) const;

    bool is_more_general_than(const AbstractState &other) const;

public:
    ~AbstractState() = default;

    AbstractState(const AbstractState &) = delete;
    AbstractState &operator=(const AbstractState &) = delete;

    AbstractState(AbstractState &&other);

    // Return the size of var's abstract domain for this state.
    int count(int var) const;

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

    bool includes(const State &concrete_state) const;

    void set_h_value(int new_h);
    int get_h_value() const;

    const Arcs &get_outgoing_arcs() const {return outgoing_arcs; }
    const Arcs &get_incoming_arcs() const {return incoming_arcs; }
    const Loops &get_loops() const {return loops; }

    AbstractSearchInfo &get_search_info() {return search_info; }

    friend std::ostream &operator<<(std::ostream &os, const AbstractState &state) {
        return os << state.domains;
    }

    /*
      Create the initial unrefined abstract state on the heap. Must be deleted
      by the caller.

      TODO: Return unique_ptr?
    */
    static AbstractState *get_trivial_abstract_state(
        const TaskProxy &task_proxy, Node *root_node);

    // Create the Cartesian set that corresponds to the given fact conditions.
    static AbstractState get_abstract_state(
        const TaskProxy &task_proxy, const ConditionsProxy &conditions);
};
}

#endif
