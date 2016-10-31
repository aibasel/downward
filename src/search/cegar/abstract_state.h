#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "domains.h"
#include "transition.h"

#include <string>
#include <utility>
#include <vector>

class ConditionsProxy;
class OperatorProxy;
class State;
class TaskProxy;

namespace cegar {
class AbstractState;
class Node;

using Transitions = std::vector<Transition>;

// To save space we store self-loops (operator indices) separately.
using Loops = std::vector<int>;

class AbstractSearchInfo {
    int g;
    Transition incoming_transition;

    static const int UNDEFINED_OPERATOR;

public:
    AbstractSearchInfo()
        : incoming_transition(UNDEFINED_OPERATOR, nullptr) {
        reset();
    }

    void reset() {
        g = std::numeric_limits<int>::max();
        incoming_transition = Transition(UNDEFINED_OPERATOR, nullptr);
    }

    void decrease_g_value_to(int new_g) {
        assert(new_g <= g);
        g = new_g;
    }

    int get_g_value() const {
        return g;
    }

    void set_incoming_transition(const Transition &transition) {
        incoming_transition = transition;
    }

    const Transition &get_incoming_transition() const {
        assert(incoming_transition.op_id != UNDEFINED_OPERATOR &&
               incoming_transition.target);
        return incoming_transition;
    }
};

/*
  Store and update abstract Domains and transitions.
*/
class AbstractState {
    // Abstract domains for all variables.
    Domains domains;

    // This state's node in the refinement hierarchy.
    Node *node;

    // Transitions from and to other abstract states.
    Transitions incoming_transitions;
    Transitions outgoing_transitions;

    // Self-loops.
    Loops loops;

    AbstractSearchInfo search_info;

    // Construct instances with factory methods.
    AbstractState(const Domains &domains, Node *node);

    void remove_non_looping_transition(
        Transitions &transitions, int op_id, AbstractState *other);

    bool is_more_general_than(const AbstractState &other) const;

public:
    void add_outgoing_transition(int op_id, AbstractState *target);
    void add_incoming_transition(int op_id, AbstractState *src);
    void add_loop(int op_id);

    void remove_incoming_transition(int op_id, AbstractState *other);
    void remove_outgoing_transition(int op_id, AbstractState *other);

    bool domains_intersect(const AbstractState *other, int var) const;

    AbstractState(const AbstractState &) = delete;

    AbstractState(AbstractState &&other);

    // Return the size of var's abstract domain for this state.
    int count(int var) const;

    bool contains(int var, int value) const;

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

    const Transitions &get_outgoing_transitions() const {
        return outgoing_transitions;
    }

    const Transitions &get_incoming_transitions() const {
        return incoming_transitions;
    }

    const Loops &get_loops() const {
        return loops;
    }

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
