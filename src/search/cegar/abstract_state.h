#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "split_tree.h"
#include "utils.h"

#include <string>
#include <utility>
#include <vector>

namespace cegar {
class AbstractState;
class ConcreteState;
class LandmarkTask;
class Values;
typedef std::pair<const GlobalOperator *, AbstractState *> Arc;
typedef std::vector<Arc> Arcs;
typedef std::vector<const GlobalOperator *> Loops;

class AbstractState {
private:
    // Forbid copy constructor and copy assignment operator.
    AbstractState(const AbstractState &);
    AbstractState &operator=(const AbstractState &);

    const LandmarkTask *task;
    const TaskProxy task_proxy;

    // Abstract domains for each variable.
    Values *values;

    // Incoming and outgoing transitions grouped by the abstract state they come
    // from or go to.
    Arcs arcs_out, arcs_in;

    // Self-loops.
    Loops loops;

    // Incumbent distance to first expanded node in backwards and forward search.
    int distance;

    const GlobalOperator *prev_solution_op;
    const GlobalOperator *next_solution_op;
    AbstractState *prev_solution_state;
    AbstractState *next_solution_state;

    // This state's node in the refinement hierarchy.
    Node *node;

    void remove_arc(Arcs &arcs, const GlobalOperator *op, AbstractState *other);
    void update_incoming_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_loops(int var, AbstractState *v1, AbstractState *v2);

    bool domains_intersect(const AbstractState *other, int var);

public:
    explicit AbstractState(TaskProxy task_proxy);
    ~AbstractState();

    // Let "result" be the set of states in which applying "op" leads to this state.
    void regress(const GlobalOperator &op, AbstractState *result) const;

    // Return the size of var's abstract domain for this state.
    size_t count(int var) const;

    // Separate the values in "wanted" from the other values in the abstract domain.
    void split(int var, std::vector<int> wanted, AbstractState *v1, AbstractState *v2);

    void add_arc(const GlobalOperator *op, AbstractState *other);
    void remove_next_arc(const GlobalOperator *op, AbstractState *other);
    void remove_prev_arc(const GlobalOperator *op, AbstractState *other);
    void add_loop(const GlobalOperator *op);

    void get_possible_splits(const AbstractState &desired,
                             const ConcreteState &prev_conc_state,
                             Splits *conditions) const;

    bool is_abstraction_of(const ConcreteState &conc_state) const;
    bool is_abstraction_of(const AbstractState &abs_state) const;
    bool is_abstraction_of_goal() const;

    // Return fraction of concrete states this state abstracts.
    double get_rel_conc_states() const;

    // A* search.
    void set_distance(int dist) {distance = dist; }
    int get_distance() {return distance; }
    void set_h(int dist) {node->set_h(dist); }
    int get_h() {return node->get_h(); }
    void set_prev_solution_op(const GlobalOperator *prev) {prev_solution_op = prev; }
    void set_next_solution_op(const GlobalOperator *next) {next_solution_op = next; }
    const GlobalOperator *get_prev_solution_op() const {return prev_solution_op; }
    const GlobalOperator *get_next_solution_op() const {return next_solution_op; }
    void set_prev_solution_state(AbstractState *prev) {prev_solution_state = prev; }
    void set_next_solution_state(AbstractState *next) {next_solution_state = next; }
    AbstractState *get_prev_solution_state() const {return prev_solution_state; }
    AbstractState *get_next_solution_state() const {return next_solution_state; }

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
