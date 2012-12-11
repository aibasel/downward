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
      h(0),
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
    ostringstream oss;
    oss << "<" << values->str() << ">";
    return oss.str();
}

void AbstractState::set_value(int var, int value) {
    values->set(var, value);
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

void AbstractState::get_unmet_conditions(const AbstractState &desired,
                                         const State &prev_conc_state,
                                         vector<pair<int, int> > *conditions)
const {
    values->get_unmet_conditions(*desired.values, prev_conc_state, conditions);
}

int AbstractState::count(int var) const {
    return values->count(var);
}

bool AbstractState::can_refine(int var, int value) const {
    return values->test(var, value) && values->count(var) >= 2;
}

void AbstractState::refine(int var, int value, AbstractState *v1, AbstractState *v2,
                           bool use_new_arc_check) {
    // We can only refine for vars that can have at least two values.
    // The desired value has to be in the set of possible values.
    assert(can_refine(var, value));

    *v1->values = *values;
    *v2->values = *values;

    // In v1 var can have all of the previous values except the desired one.
    v1->values->remove(var, value);

    // In v2 var can only have the desired value.
    v2->values->set(var, value);

    // Results from Dijkstra search. If  u --> v --> w  was on the
    // shortest path and a new path  u --> v{1,2} --> w is created with the
    // same arcs, we avoid a dijkstra computation.
    bool u_v1 = false, u_v2 = false, v1_w = false, v2_w = false;

    // Before: u --> this=v --> w
    //  ==>
    // After:  v is split into v1 and v2
    typedef boost::unordered_map<AbstractState *, bool> map_type;
    map_type intersects_with;
    map_type::iterator intersect_iter;
    Arcs::iterator it;
    for (it = prev.begin(); it != prev.end(); ++it) {
        Operator *op = it->first;
        AbstractState *u = it->second;
        assert(u != this);
        bool is_solution_arc = ((op == op_in) && (u == state_in));
        // If the first check returns false, the second arc has to be added.
        if (use_new_arc_check) {
            int post = get_post(*op, var);
            if (post == UNDEFINED) {
                bool intersects = false;
                intersect_iter = intersects_with.find(u);
                if (intersect_iter != intersects_with.end()) {
                    // Value for u already exists.
                    intersects = intersect_iter->second;
                } else {
                    // There's no value for u in the map.
                    intersects = u->values->domains_intersect(*(v1->values), var);
                    intersects_with.insert(make_pair(u, intersects));
                }
                if (intersects) {
                    u->add_arc(op, v1);
                    u_v1 |= is_solution_arc;
                }
                if (u->values->test(var, value)) {
                    u->add_arc(op, v2);
                    u_v2 |= is_solution_arc;
                }
            } else if (post == value) {
                u->add_arc(op, v2);
                u_v2 |= is_solution_arc;
            } else {
                u->add_arc(op, v1);
                u_v1 |= is_solution_arc;
            }
        } else {
            if (u->check_and_add_arc(op, v1)) {
                bool added = u->check_and_add_arc(op, v2);
                if (is_solution_arc) {
                    u_v1 = true;
                    u_v2 |= added;
                }
            } else {
                u->add_arc(op, v2);
                u_v2 |= is_solution_arc;
            }
        }
        u->remove_next_arc(op, this);
    }
    for (it = next.begin(); it != next.end(); ++it) {
        Operator *op = it->first;
        AbstractState *w = it->second;
        assert(w != this);
        bool is_solution_arc = ((op == op_out) && (w == state_out));
        if (use_new_arc_check) {
            int pre = get_pre(*op, var);
            int post = get_post(*op, var);
            if (post == UNDEFINED) {
                bool intersects = false;
                intersect_iter = intersects_with.find(w);
                if (intersect_iter != intersects_with.end()) {
                    // Value for w already exists.
                    intersects = intersect_iter->second;
                } else {
                    // There's no value for w in the map.
                    intersects = v1->values->domains_intersect(*w->values, var);
                    intersects_with.insert(make_pair(w, intersects));
                }
                if (intersects) {
                    v1->add_arc(op, w);
                    v1_w |= is_solution_arc;
                }
                if (w->values->test(var, value)) {
                    v2->add_arc(op, w);
                    v2_w |= is_solution_arc;
                }
            } else if (pre == UNDEFINED) {
                v1->add_arc(op, w);
                v2->add_arc(op, w);
                v1_w |= is_solution_arc;
                v2_w |= is_solution_arc;
            } else if (pre == value) {
                v2->add_arc(op, w);
                v2_w |= is_solution_arc;
            } else {
                v1->add_arc(op, w);
                v1_w |= is_solution_arc;
            }
        } else {
            // If the first check returns false, the second arc has to be added.
            if (v1->check_and_add_arc(op, w)) {
                bool added = v2->check_and_add_arc(op, w);
                if (is_solution_arc) {
                    v1_w = true;
                    v2_w |= added;
                }
            } else {
                v2->add_arc(op, w);
                v2_w |= is_solution_arc;
            }
        }
        w->remove_prev_arc(op, this);
    }
    for (int i = 0; i < loops.size(); ++i) {
        Operator *op = loops[i];
        if (use_new_arc_check) {
            int pre = get_pre(*op, var);
            int post = get_post(*op, var);
            if (pre == UNDEFINED) {
                if (post == UNDEFINED) {
                    v1->add_loop(op);
                    v2->add_loop(op);
                } else if (post == value) {
                    v1->add_arc(op, v2);
                    v2->add_loop(op);
                } else {
                    assert(v1->values->test(var, post));
                    v1->add_loop(op);
                    v2->add_arc(op, v1);
                }
            } else if (pre == value) {
                assert(post != UNDEFINED);
                if (post == value) {
                    v2->add_loop(op);
                } else {
                    assert(v1->values->test(var, post));
                    v2->add_arc(op, v1);
                }
            } else if (post == value) {
                v1->add_arc(op, v2);
            } else {
                assert(v1->values->test(var, post));
                v1->add_loop(op);
            }
        } else {
            v1->check_and_add_arc(op, v2);
            v2->check_and_add_arc(op, v1);
            v1->check_and_add_loop(op);
            v2->check_and_add_loop(op);
        }
    }

    assert(v1->values->count(var) == values->count(var) - 1);
    assert(v2->values->count(var) == 1);

    // Check that the sets of possible values are now smaller.
    assert(this->is_abstraction_of(*v1));
    assert(this->is_abstraction_of(*v2));

    // Pass on the h-value.
    v1->set_h(h);
    v2->set_h(h);

    if (u_v1 && u_v2)
        assert(state_in->can_refine(var, value));

    // Update the solution path. There may now be two paths (over v1 and v2).
    if (u_v1) v1->set_predecessor(op_in, state_in);
    if (u_v2) v2->set_predecessor(op_in, state_in);
    if (v1_w) v1->set_successor(op_out, state_out);
    if (v2_w) v2->set_successor(op_out, state_out);

    /*
    // Update the path from u to one of the new states. If both paths are valid,
    // we can take either. Only if one state abstracts the goal, we should
    // choose it. The operator obviously doesn't have to be changed.
    if (u_v1 && (!u_v2 || v1->is_abstraction_of_goal())) {
        state_in->state_out = v1;
    } else if (u_v2) {
        state_in->state_out = v2;
    }

    // Update the path from v1 and v2 to w. If both paths are valid, we can take
    // either. Only if one state abstracts the initial state, we should choose
    // it. The operator obviously doesn't have to be changed.
    if (v1_w && (!v2_w || v1->is_abstraction_of(*g_initial_state))) {
        state_out->state_in = v1;
    } else if (v2_w) {
        state_out->state_in = v2;
    }
    */
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

bool AbstractState::check_arc(Operator *op, AbstractState *other) {
    // Using a vector<bool> here is faster than using a bitset.
    vector<bool> checked(g_variable_domain.size(), false);
    for (int i = 0; i < op->get_prevail().size(); ++i) {
        const Prevail &prevail = op->get_prevail()[i];
        const int &var = prevail.var;
        const int &value = prevail.prev;
        // Check if operator is applicable.
        assert(value != -1);
        if (!values->test(var, value))
            return false;
        // Check if we land in the desired state.
        // If this == other we have already done the check above.
        if ((this != other) && (!other->values->test(var, value)))
            return false;
        checked[var] = true;
    }
    for (int i = 0; i < op->get_pre_post().size(); ++i) {
        // Check if pre value is in the set of possible values.
        const PrePost &prepost = op->get_pre_post()[i];
        const int &var = prepost.var;
        const int &pre = prepost.pre;
        const int &post = prepost.post;
        assert(prepost.cond.empty());
        assert(!checked[var]);
        // Check if operator is applicable.
        if ((pre != -1) && !values->test(var, pre))
            return false;
        // Check if we land in the desired state.
        if (!other->values->test(var, post))
            return false;
        checked[var] = true;
    }
    if (this != other) {
        return values->all_vars_intersect(*other->values, checked);
    }
    return true;
}

bool AbstractState::check_and_add_arc(Operator *op, AbstractState *other) {
    if (check_arc(op, other)) {
        add_arc(op, other);
        return true;
    }
    return false;
}

bool AbstractState::check_and_add_loop(Operator *op) {
    if (check_arc(op, this)) {
        add_loop(op);
        return true;
    }
    return false;
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

AbstractState *AbstractState::get_child(int value) {
    assert(node);
    return node->get_child(value)->get_abs_state();
}

AbstractState *AbstractState::get_left_child() const {
    Node *child = node->get_left_child();
    assert(child);
    // Jump helper nodes.
    while (!child->get_abs_state()) {
        child = child->get_left_child();
    }
    assert(child->get_abs_state());
    return child->get_abs_state();
}

AbstractState *AbstractState::get_right_child() const {
    return node->get_right_child()->get_abs_state();
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

long AbstractState::get_size() const {
    // All three lists consist occupy the same size when they're empty (12 bit
    // for 32bit compilations.
    return 2 * sizeof(Arcs) + sizeof(Arc) * (next.capacity() + prev.capacity()) +
           sizeof(Loops) + sizeof(Operator*) * loops.capacity();
}
}
