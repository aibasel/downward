#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "split_tree.h"
#include "utils.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cegar_heuristic {
class AbstractState;
class Values;
typedef pair<Operator *, AbstractState *> Arc;
typedef std::vector<Arc> Arcs;
typedef std::vector<Operator *> Loops;
typedef std::map<AbstractState *, Operators> StatesToOps;

class AbstractState {
private:
    // Forbid copy constructor and copy assignment operator.
    AbstractState(const AbstractState &);
    AbstractState &operator=(const AbstractState &);

    // Abstract domains for each variable.
    Values *values;

    // Incoming and outgoing transitions grouped by the abstract state they come
    // from or go to.
    StatesToOps arcs_out, arcs_in;

    // Self-loops.
    Loops loops;

    // Incumbent distance to first expanded node in backwards and forward search.
    int distance;

    // Record the neighbours on a solution path.
    Arcs solution_in, solution_out;

    // This state's node in the refinement hierarchy.
    Node *node;

    void remove_arc(StatesToOps &arcs, Operator *op, AbstractState *other);
    void update_incoming_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2);
    void update_loops(int var, AbstractState *v1, AbstractState *v2);

public:
    explicit AbstractState(string s = "");
    ~AbstractState();

    // Let "result" be the set of states in which applying "op" leads to this state.
    FRIEND_TEST(CegarTest, regress);
    void regress(const Operator &op, AbstractState *result) const;

    // Return the size of var's abstract domain for this state.
    int count(int var) const;

    // Separate the values in "wanted" from the other values in the abstract domain.
    void split(int var, vector<int> wanted, AbstractState *v1, AbstractState *v2);

    void add_arc(Operator *op, AbstractState *other);
    void remove_next_arc(Operator *op, AbstractState *other);
    void remove_prev_arc(Operator *op, AbstractState *other);
    void add_loop(Operator *op);

    void get_possible_splits(const AbstractState &desired, const State &prev_conc_state,
                             Splits *conditions) const;

    bool is_abstraction_of(const State &conc_state) const;
    bool is_abstraction_of(const AbstractState &abs_state) const;
    bool is_abstraction_of_goal() const;

    // Return fraction of concrete states this state abstracts.
    double get_rel_conc_states() const;

    // A* search.
    void set_distance(int dist) {distance = dist; }
    int get_distance() {return distance; }
    void set_h(int dist) {node->set_h(dist); }
    int get_h() {return node->get_h(); }
    void set_predecessor(Operator *op, AbstractState *other);
    void add_predecessor(Operator *op, AbstractState *other);
    Arcs &get_solution_in() {return solution_in; }
    void set_successor(Operator *op, AbstractState *other);
    void add_successor(Operator *op, AbstractState *other);
    Arcs &get_solution_out() {return solution_out; }
    void reset_neighbours();

    StatesToOps &get_arcs_out() {return arcs_out; };
    StatesToOps &get_arcs_in() {return arcs_in; };
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
