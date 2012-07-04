#ifndef CEGAR_ABSTRACT_STATE_H
#define CEGAR_ABSTRACT_STATE_H

#include "../operator.h"
#include "../state.h"

#include "../ext/gtest/include/gtest/gtest_prod.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// TODO: Check out boost dynamic bitset

namespace cegar_heuristic {
class AbstractState;
typedef pair<Operator *, AbstractState *> Arc;
typedef std::set<int> Domain;

extern int same;
extern int different;
extern int doubles;

class AbstractState {
private:
    // Possible values of each variable in this state.
    // values[1] == {2} -> var1 is concretely set here.
    // values[1] == {2, 3} -> var1 has two possible values.
    // values[1] == {} -> var1 has all possible values.
    std::vector<Domain> values;

    std::vector<Arc> next, prev;
    void remove_arc(std::vector<Arc> *arcs, Operator *op, AbstractState *other);

    // For Dijkstra search.
    int distance;
    int min_distance; // TODO: Rename to min_cost_to_goal
    Arc *origin;  //TODO: Rename to prev_arc
    Arc *next_arc;

    // Refinement hierarchy. Save the variable for which this state was refined
    // and the resulting abstract child states.
    int var;
    std::map<int, AbstractState *> children;
    AbstractState *left;
    AbstractState *right;

public:
    explicit AbstractState(string s = "", bool init = true);
    FRIEND_TEST(CegarTest, regress);
    void regress(const Operator &op, AbstractState *result) const;
    std::string str() const;
    const Domain &get_values(int var) const;
    void set_value(int var, int value);
    // Return a pointer to the state where the next solution check should start.
    // Return 0 if the search has to be started from the beginning.
    AbstractState *refine(int var, int value, AbstractState *v1, AbstractState *v2);
    bool operator==(const AbstractState &other) const;
    bool operator!=(const AbstractState &other) const;
    void add_arc(Operator *op, AbstractState *other);
    void remove_next_arc(Operator *op, AbstractState *other);
    void remove_prev_arc(Operator *op, AbstractState *other);
    bool check_and_add_arc(Operator *op, AbstractState *other);
    bool check_arc(Operator *op, AbstractState *other);
    bool applicable(const Operator &op) const;
    void apply(const Operator &op, AbstractState *result) const;
    bool agrees_with(const AbstractState &other) const;
    void get_unmet_conditions(const AbstractState &desired, vector<pair<int, int> > *conditions) const;
    bool is_abstraction_of(const State &conc_state) const;
    bool is_abstraction_of(const AbstractState &abs_state) const;
    bool is_abstraction_of_goal() const;

    void set_distance(int dist) {distance = dist; }
    int get_distance() {return distance; }
    void set_min_distance(int dist) {min_distance = dist; }
    int get_min_distance() {return min_distance; }
    void set_origin(Arc *orig) {delete origin;
                                origin = orig; }
    Arc *get_origin() {return origin; }
    void set_next_arc(Arc *next) {delete next_arc;
                                  next_arc = next; }
    Arc *get_next_arc() {return next_arc; }

    std::vector<Arc> &get_next() {return next; }
    std::string get_next_as_string() const;
    std::vector<Arc> &get_prev() {return prev; }

    // We only have a valid abstract state if it was not refined.
    bool valid() const;
    int get_var() const;
    // TODO: Why can't we make this method const?
    AbstractState *get_child(int value);
    AbstractState *get_left_child() const;
    AbstractState *get_right_child() const;
};
}

#endif
