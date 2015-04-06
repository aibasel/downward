#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "split_tree.h"
#include "utils.h"
#include "values.h"

#include "../task_proxy.h"

#include <string>
#include <utility>
#include <vector>

namespace cegar {
class AbstractState;

using Arc = std::pair<OperatorProxy, AbstractState *>;
using Arcs = std::vector<Arc>;
using Loops = std::vector<OperatorProxy>;

class AbstractState {
private:
    const TaskProxy task_proxy;

    // Abstract domains for each variable.
    Values values;

    // Incoming and outgoing transitions grouped by the abstract state they come
    // from or go to.
    Arcs arcs_out, arcs_in;

    // Self-loops.
    Loops loops;

    // Incumbent distance to first expanded node in backwards and forward search.
    int distance;

    // This state's node in the refinement hierarchy.
    Node *node;

    void remove_arc(Arcs &arcs, OperatorProxy op, AbstractState *other);
    void update_incoming_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_loops(int var, AbstractState *v1, AbstractState *v2);

    bool domains_intersect(const AbstractState *other, int var);

public:
    explicit AbstractState(TaskProxy task_proxy);
    ~AbstractState() = default;

    AbstractState(const AbstractState &) = delete;
    AbstractState &operator=(const AbstractState &) = delete;

    // Let "result" be the set of states in which applying "op" leads to this state.
    void regress(OperatorProxy op, AbstractState *result) const;

    // Return the size of var's abstract domain for this state.
    size_t count(int var) const;

    // Separate the values in "wanted" from the other values in the abstract domain.
    void split(int var, std::vector<int> wanted, AbstractState *v1, AbstractState *v2);

    void add_arc(OperatorProxy op, AbstractState *other);
    void remove_next_arc(OperatorProxy op, AbstractState *other);
    void remove_prev_arc(OperatorProxy op, AbstractState *other);
    void add_loop(OperatorProxy op);

    void get_possible_splits(const AbstractState &desired,
                             const State &prev_conc_state,
                             Splits *conditions) const;

    bool is_abstraction_of(const State &conc_state) const;
    bool is_abstraction_of(const AbstractState &abs_state) const;

    // A* search.
    void set_distance(int dist) {distance = dist; }
    int get_distance() {return distance; }
    void set_h(int dist) {node->set_h(dist); }
    int get_h() {return node->get_h(); }

    Arcs &get_arcs_out() {return arcs_out; }
    Arcs &get_arcs_in() {return arcs_in; }
    Loops &get_loops() {return loops; }

    Node *get_node() const {return node; }
    void set_node(Node *node) {
        assert(!this->node);
        this->node = node;
    }

    // Testing.
    std::string str() const;
};
}

#endif
