#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "utils.h"
#include "../operator.h"
#include "../state.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cegar_heuristic {
class AbstractState;
typedef pair<Operator *, AbstractState *> Arc;

class AbstractState {
private:
    // Forbid copy constructor and copy assignment operator.
    AbstractState(const AbstractState &);
    AbstractState &operator=(const AbstractState &);

    // Possible values of each variable in this state.
    // values[1] == {2} -> var1 is concretely set here.
    // values[1] == {2, 3} -> var1 has two possible values.
    std::vector<Domain> values;

    std::vector<Arc> next, prev;
    void remove_arc(std::vector<Arc> *arcs, Operator *op, AbstractState *other);

    // Incumbent distance to first expanded node in backwards and forward search.
    int distance;
    // Estimated cost to goal node.
    int h;

    // Record the neighbours on a solution path.
    Operator *op_in;
    AbstractState *state_in;
    Operator *op_out;
    AbstractState *state_out;

    // Refinement hierarchy. Save the variable for which this state was refined
    // and the resulting abstract child states.
    int refined_var;
    int refined_value;
    AbstractState *left_child;
    AbstractState *right_child;

public:
    explicit AbstractState(string s = "");
    FRIEND_TEST(CegarTest, regress);
    void regress(const Operator &op, AbstractState *result) const;
    std::string str() const;
    const Domain &get_values(int var) const;
    void set_value(int var, int value);
    // Return a pointer to the state where the next solution check should start.
    // Return 0 if the search has to be started from the beginning.
    AbstractState *refine(int var, int value, AbstractState *v1, AbstractState *v2);
    bool refinement_breaks_shortest_path(int var, int value) const;
    void add_arc(Operator *op, AbstractState *other);
    void remove_next_arc(Operator *op, AbstractState *other);
    void remove_prev_arc(Operator *op, AbstractState *other);
    bool check_arc(Operator *op, AbstractState *other);
    bool check_and_add_arc(Operator *op, AbstractState *other);
    void get_unmet_conditions(const AbstractState &desired, vector<pair<int, int> > *conditions) const;
    bool is_abstraction_of(const State &conc_state) const;
    bool is_abstraction_of(const AbstractState &abs_state) const;
    bool is_abstraction_of_goal() const;

    // A* search.
    void set_distance(int dist) {distance = dist; }
    int get_distance() {return distance; }
    void set_h(int dist) {h = dist; }
    int get_h() {return h; }
    void set_predecessor(Operator *op, AbstractState *other);
    Operator *get_op_in() const {return op_in; }
    AbstractState *get_state_in() const {return state_in; }
    void set_successor(Operator *op, AbstractState *other);
    Operator *get_op_out() const {return op_out; }
    AbstractState *get_state_out() const {return state_out; }
    void reset_neighbours();

    std::vector<Arc> &get_next() {return next; }
    std::string get_next_as_string() const;
    std::vector<Arc> &get_prev() {return prev; }

    // We only have a valid abstract state if it was not refined.
    bool valid() const;
    int get_refined_var() const;
    AbstractState *get_child(int value);

    void release_memory();
};
}

#endif
