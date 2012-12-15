#include "abstract_state.h"

#include "values.h"

#include "../globals.h"
#include "../operator.h"
#include "../state.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include <boost/unordered_map.hpp>

using namespace std;

namespace cegar_heuristic {
AbstractState::AbstractState(string s)
    : values(new Values()),
      distance(UNDEFINED),
      node(0) {
    assert(!g_variable_domain.empty());

    reset_neighbours();

    if (s.empty())
        return;

    // Construct state from string s of the form "<0={0,1}>".
    istringstream iss(s);
    char next;
    int current_var = UNDEFINED;
    int current_val = UNDEFINED;
    bool in_bracket = false;
    while (!iss.eof()) {
        // iss.peek() and iss.get() return strange chars at beginning and end
        // of stream.
        iss >> next;
        if (next == '<' || next == '>' || next == '=' || next == ',') {
            // skip.
        } else if (next == '{') {
            in_bracket = true;
        } else if (next == '}') {
            in_bracket = false;
        } else if ((next >= '0') && (next <= '9')) {
            // We can use iss.unget(), because next is a single char.
            iss.unget();
            if (in_bracket) {
                iss >> current_val;
                values->add(current_var, current_val);
            } else {
                iss >> current_var;
                values->remove_all(current_var);
            }
        }
    }
}

AbstractState::~AbstractState() {
    delete values;
    values = 0;
}

string AbstractState::str() const {
    assert(values);
    ostringstream oss;
    oss << "<" << values->str() << ">";
    return oss.str();
}

void AbstractState::regress(const Operator &op, AbstractState *result) const {
    // If v does NOT occur in op.pre and NOT in op.post we use the values from
    // this state.
    *result->values = *values;
    for (int v = 0; v < g_variable_domain.size(); ++v) {
        int pre = get_pre(op, v);
        int post = get_post(op, v);
        if (pre != UNDEFINED) {
            result->values->set(v, pre);
        } else if (post != UNDEFINED) {
            assert(values->test(v, post));
            result->values->add_all(v);
        }
    }
}

void AbstractState::get_possible_splits(const AbstractState &desired,
                                        const State &prev_conc_state,
                                        Splits *splits)
const {
    values->get_possible_splits(*desired.values, prev_conc_state, splits);
}

int AbstractState::count(int var) const {
    return values->count(var);
}

void AbstractState::split(int var, vector<int> wanted, AbstractState *v1, AbstractState *v2) {
    // We can only refine for vars that can have at least two values.
    // The desired value has to be in the set of possible values.
    assert(wanted.size() >= 1);
    assert(values->count(var) > wanted.size());
    for (int i = 0; i < wanted.size(); ++i)
        assert(values->test(var, wanted[i]));

    *v1->values = *values;
    *v2->values = *values;

    v2->values->remove_all(var);
    for (int i = 0; i < wanted.size(); ++i) {
        // In v1 var can have all of the previous values except the wanted ones.
        v1->values->remove(var, wanted[i]);

        // In v2 var can only have the wanted values.
        v2->values->add(var, wanted[i]);
    }

    // u --> v --> w  was on the shortest path. We check if we can form paths
    // with the same transitions over v1 and v2. If both paths u_v1 and u_v2
    // are valid, we do a cascaded refinement.
    bool u_v1 = false, u_v2 = false, v1_w = false, v2_w = false;

    // Before: u --> this=v --> w
    //  ==>
    // After:  v is split into v1 and v2
    Arcs::iterator it;
    for (it = prev.begin(); it != prev.end(); ++it) {
        Operator *op = it->first;
        AbstractState *u = it->second;
        assert(u != this);
        bool is_solution_arc = ((op == op_in) && (u == state_in));
        int post = get_post(*op, var);
        if (post == UNDEFINED) {
            if (u->values->domains_intersect(*(v1->values), var)) {
                u->add_arc(op, v1);
                u_v1 |= is_solution_arc;
            }
            if (u->values->domains_intersect(*(v2->values), var)) {
                u->add_arc(op, v2);
                u_v2 |= is_solution_arc;
            }
        } else if (v2->values->test(var, post)) {
            u->add_arc(op, v2);
            u_v2 |= is_solution_arc;
        } else {
            u->add_arc(op, v1);
            u_v1 |= is_solution_arc;
        }
        u->remove_next_arc(op, this);
    }
    for (it = next.begin(); it != next.end(); ++it) {
        Operator *op = it->first;
        AbstractState *w = it->second;
        assert(w != this);
        bool is_solution_arc = ((op == op_out) && (w == state_out));
        int pre = get_pre(*op, var);
        int post = get_post(*op, var);
        if (post == UNDEFINED) {
            if (v1->values->domains_intersect(*w->values, var)) {
                v1->add_arc(op, w);
                v1_w |= is_solution_arc;
            }
            if (v2->values->domains_intersect(*w->values, var)) {
                v2->add_arc(op, w);
                v2_w |= is_solution_arc;
            }
        } else if (pre == UNDEFINED) {
            v1->add_arc(op, w);
            v2->add_arc(op, w);
            v1_w |= is_solution_arc;
            v2_w |= is_solution_arc;
        } else if (v2->values->test(var, pre)) {
            v2->add_arc(op, w);
            v2_w |= is_solution_arc;
        } else {
            v1->add_arc(op, w);
            v1_w |= is_solution_arc;
        }
        w->remove_prev_arc(op, this);
    }
    for (int i = 0; i < loops.size(); ++i) {
        Operator *op = loops[i];
        int pre = get_pre(*op, var);
        int post = get_post(*op, var);
        if (pre == UNDEFINED) {
            if (post == UNDEFINED) {
                v1->add_loop(op);
                v2->add_loop(op);
            } else if (v2->values->test(var, post)) {
                v1->add_arc(op, v2);
                v2->add_loop(op);
            } else {
                assert(v1->values->test(var, post));
                v1->add_loop(op);
                v2->add_arc(op, v1);
            }
        } else if (v2->values->test(var, pre)) {
            assert(post != UNDEFINED);
            if (v2->values->test(var, post)) {
                v2->add_loop(op);
            } else {
                assert(v1->values->test(var, post));
                v2->add_arc(op, v1);
            }
        } else if (v2->values->test(var, post)) {
            v1->add_arc(op, v2);
        } else {
            assert(v1->values->test(var, post));
            v1->add_loop(op);
        }
    }

    assert(v1->values->count(var) == values->count(var) - wanted.size());
    assert(v2->values->count(var) == wanted.size());

    // Check that the sets of possible values are now smaller.
    assert(this->is_abstraction_of(*v1));
    assert(this->is_abstraction_of(*v2));

    // Update the solution path. There may now be two paths (over v1 and v2).
    if (u_v1) v1->set_predecessor(op_in, state_in);
    if (u_v2) v2->set_predecessor(op_in, state_in);
    if (v1_w) v1->set_successor(op_out, state_out);
    if (v2_w) v2->set_successor(op_out, state_out);
}

void AbstractState::add_arc(Operator *op, AbstractState *other) {
    // Experiments showed that keeping the arcs sorted for faster removal
    // increases the overall processing time. In 30 domains it made no
    // difference for 10 domains, 17 domains preferred unsorted arcs and in
    // 3 domains performance was better with sorted arcs.
    // Inlining this method has no effect either.
    assert(other != this);
    next.push_back(Arc(op, other));
    other->prev.push_back(Arc(op, this));
}

void AbstractState::add_loop(Operator *op) {
    loops.push_back(op);
}

void AbstractState::remove_arc(Arcs &arcs, Operator *op, AbstractState *other) {
    // Move arcs.back() to pos to speed things up.
    Arcs::iterator pos = find(arcs.begin(), arcs.end(), Arc(op, other));
    assert(pos != arcs.end());
    // For PODs assignment is faster than swapping.
    *pos = arcs.back();
    arcs.pop_back();
}

void AbstractState::remove_next_arc(Operator *op, AbstractState *other) {
    remove_arc(next, op, other);
}

void AbstractState::remove_prev_arc(Operator *op, AbstractState *other) {
    remove_arc(prev, op, other);
}

bool AbstractState::is_abstraction_of(const State &conc_state) const {
    // Return true if every concrete value is contained in the possible values.
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        if (!values->test(var, conc_state[var]))
            return false;
    }
    return true;
}

bool AbstractState::is_abstraction_of(const AbstractState &other) const {
    // Return true if all our possible value sets are supersets of the
    // other's respective sets.
    return values->abstracts(*other.values);
}

bool AbstractState::is_abstraction_of_goal() const {
    assert(!g_goal.empty());
    for (int i = 0; i < g_goal.size(); ++i) {
        if (!values->test(g_goal[i].first, g_goal[i].second))
            return false;
    }
    return true;
}

void AbstractState::set_predecessor(Operator *op, AbstractState *other) {
    op_in = op;
    state_in = other;
}

void AbstractState::set_successor(Operator *op, AbstractState *other) {
    op_out = op;
    state_out = other;
}

void AbstractState::reset_neighbours() {
    op_in = 0, state_in = 0, op_out = 0, state_out = 0;
}

double AbstractState::get_rel_conc_states() const {
    double fraction = 1.0;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        const int domain_size = values->count(var);
        assert(domain_size >= 1);
        fraction *= double(domain_size) / g_variable_domain[var];
    }
    assert(fraction <= 1.0);
    assert(fraction > 0.0);
    return fraction;
}
}
