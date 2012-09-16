#include "abstract_state.h"

#include "../globals.h"
#include "../operator.h"
#include "../state.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

using namespace std;

namespace cegar_heuristic {
AbstractState::AbstractState(string s)
    : distance(UNDEFINED),
      h(0),
      refined_var(UNDEFINED),
      refined_value(UNDEFINED),
      left_child(0),
      right_child(0) {
    assert(!g_variable_domain.empty());

    reset_neighbours();

    for (int var = 0; var < g_variable_domain.size(); ++var) {
        Domain domain(g_variable_domain[var]);
        domain.set();
        values.push_back(domain);
    }

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
                values[current_var].set(current_val);
            } else {
                iss >> current_var;
                values[current_var].reset();
            }
        }
    }
}

string AbstractState::str() const {
    ostringstream oss;
    string sep = "";
    oss << "<";
    for (int i = 0; i < values.size(); ++i) {
        const Domain &vals = values[i];
        if (vals.count() != g_variable_domain[i]) {
            oss << sep << i << "=" << domain_to_string(vals);
            sep = ",";
        }
    }
    oss << ">";
    return oss.str();
}

string AbstractState::get_next_as_string() const {
    // Format: [(op.name,state.str),...]
    ostringstream oss;
    string sep = "";
    oss << "[";
    for (int i = 0; i < next.size(); ++i) {
        Operator *op = next[i].first;
        AbstractState *abs = next[i].second;
        oss << sep << "(" << op->get_name() << "," << abs->str() << ")";
        sep = ",";
    }
    oss << "]";
    return oss.str();
}

const Domain &AbstractState::get_values(int var) const {
    return values[var];
}

void AbstractState::set_value(int var, int value) {
    values[var].reset();
    values[var].set(value);
}

void AbstractState::regress(const Operator &op, AbstractState *result) const {
    for (int v = 0; v < g_variable_domain.size(); ++v) {
        // s1_vals = s2_vals if v does NOT occur in op.pre and NOT in op.eff
        Domain s1_vals(values[v]);
        // if v occurs in op.eff:
        int eff = get_eff(op, v);
        if (eff != UNDEFINED) {
            assert(values[v][eff]);
            // s1_vals = v.all_values_in_domain
            s1_vals.set();
        }
        // if v occurs in op.pre:
        int pre = get_pre(op, v);
        if (pre != UNDEFINED) {
            assert(s1_vals[pre]);
            // s1_vals = [op.pre[v]]
            s1_vals.reset();
            s1_vals.set(pre);
        }
        result->values[v] = s1_vals;
    }
}

void AbstractState::get_unmet_conditions(const AbstractState &desired,
                                         vector<pair<int, int> > *conditions)
const {
    // Get all set intersections of the possible values here with the possible
    // values in "desired".
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        Domain intersection = values[i] & desired.values[i];
        if (intersection.count() < values[i].count()) {
            // The variable's value matters for determining the resulting state.
            int next_value = intersection.find_first();
            while (next_value != Domain::npos) {
                conditions->push_back(pair<int, int>(i, next_value));
                next_value = intersection.find_next(next_value);
            }
        }
    }
}

AbstractState *AbstractState::refine(int var, int value, AbstractState *v1, AbstractState *v2) {
    // We can only refine for vars that can have at least two values.
    assert(values[var].count() >= 2);
    // The desired value has to be in the set of possible values.
    assert(values[var][value]);

    v1->values = values;
    v2->values = values;

    // In v1 var can have all of the previous values except the desired one.
    v1->values[var].reset(value);

    // In v2 var can only have the desired value.
    v2->set_value(var, value);

    // Results from Dijkstra search. If  u --> v --> w  was on the
    // shortest path and a new path  u --> v{1,2} --> w is created with the
    // same arcs, we avoid a dijkstra computation.
    bool u_v1 = false, u_v2 = false, v1_w = false, v2_w = false;

    // Before: u --> this=v --> w
    //  ==>
    // After:  v is split into v1 and v2
    for (int i = 0; i < prev.size(); ++i) {
        Operator *op = prev[i].first;
        AbstractState *u = prev[i].second;
        assert(u != this);
        u->remove_next_arc(op, this);
        // If the first check returns false, the second arc has to be added.
        if (u->check_and_add_arc(op, v1)) {
            bool added = u->check_and_add_arc(op, v2);
            if (op == op_in && u == state_in) {
                u_v1 = true;
                u_v2 |= added;
            }
        } else {
            u->add_arc(op, v2);
            u_v2 |= (op == op_in && u == state_in);
        }
    }
    for (int i = 0; i < next.size(); ++i) {
        Operator *op = next[i].first;
        AbstractState *w = next[i].second;
        assert(w != this);
        w->remove_prev_arc(op, this);
        // If the first check returns false, the second arc has to be added.
        if (v1->check_and_add_arc(op, w)) {
            bool added = v2->check_and_add_arc(op, w);
            if (op == op_out && w == state_out) {
                v1_w = true;
                v2_w |= added;
            }
        } else {
            v2->add_arc(op, w);
            v2_w |= (op == op_out && w == state_out);
        }
    }
    for (int i = 0; i < loops.size(); ++i) {
        Operator *op = loops[i];
        v1->check_and_add_arc(op, v2);
        v2->check_and_add_arc(op, v1);
        v1->check_and_add_loop(op);
        v2->check_and_add_loop(op);
    }

    // Save the refinement hierarchy.
    refined_var = var;
    refined_value = value;
    left_child = v1;
    right_child = v2;
    assert(v1->values[var].count() == values[var].count() - 1);
    assert(v2->values[var].count() == 1);

    // Check that the sets of possible values are now smaller.
    assert(this->is_abstraction_of(*v1));
    assert(this->is_abstraction_of(*v2));

    // Remove obsolete members.
    release_memory();

    // Pass on the h-value.
    v1->set_h(h);
    v2->set_h(h);

    AbstractState *bridge_state = 0;
    // If we refine a goal state, only reuse solution if the path leads to the goal.
    bool v1_is_bridge = (u_v1 && v1_w) || (u_v1 && !state_out && v1->is_abstraction_of_goal());
    bool v2_is_bridge = (u_v2 && v2_w) || (u_v2 && !state_out && v2->is_abstraction_of_goal());
    if (v2_is_bridge) {
        // Prefer going over v2. // TODO: add option?
        bridge_state = v2;
    } else if (v1_is_bridge) {
        bridge_state = v1;
    }
    if (bridge_state) {
        assert(state_in);
        assert(op_in);
        state_in->set_successor(op_in, bridge_state);
        if (state_out) {
            assert(op_out);
            bridge_state->set_successor(op_out, state_out);
        }
        return state_in;
    }
    return 0;
}

bool AbstractState::refinement_breaks_shortest_path(int var, int value) const {
    // We can only refine for vars that can have at least two values.
    assert(values[var].count() >= 2);
    // The desired value has to be in the set of possible values.
    assert(values[var][value]);

    AbstractState v1 = AbstractState();
    AbstractState v2 = AbstractState();

    v1.values = values;
    v2.values = values;

    // In v1 var can have all of the previous values except the desired one.
    v1.values[var].reset(value);

    // In v2 var can only have the desired value.
    v2.set_value(var, value);

    bool u_v1 = (state_in) ? state_in->check_arc(op_in, &v1) : false;
    bool u_v2 = (state_in) ? state_in->check_arc(op_in, &v2) : false;
    bool v1_w = (state_out) ? v1.check_arc(op_out, state_out) : false;
    bool v2_w = (state_out) ? v2.check_arc(op_out, state_out) : false;
    return !(u_v1 && v1_w) && !(u_v1 && !state_out && v1.is_abstraction_of_goal()) &&
           !(v1_w && !state_in && v1.is_abstraction_of(*g_initial_state)) &&
           !(u_v2 && v2_w) && !(u_v2 && !state_out && v2.is_abstraction_of_goal()) &&
           !(v2_w && !state_in && v2.is_abstraction_of(*g_initial_state));
}

void AbstractState::add_arc(Operator *op, AbstractState *other) {
    assert(other != this);
    next.push_back(Arc(op, other));
    other->prev.push_back(Arc(op, this));
}

void AbstractState::add_loop(Operator *op) {
    loops.push_back(op);
}

void AbstractState::remove_arc(vector<Arc> &arcs, Operator *op, AbstractState *other) {
    for (int i = 0; i < arcs.size(); ++i) {
        Operator *current_op = arcs[i].first;
        AbstractState *current_state = arcs[i].second;
        if ((current_op == op) && (current_state == other)) {
            arcs.erase(arcs.begin() + i);
            return;
        }
    }
    cout << "REMOVE: " << str() << " " << op->get_name() << " " << other->str() << endl;
    assert(!"Trying to remove an arc that is not there.");
}

void AbstractState::remove_next_arc(Operator *op, AbstractState *other) {
    remove_arc(next, op, other);
}

void AbstractState::remove_prev_arc(Operator *op, AbstractState *other) {
    remove_arc(prev, op, other);
}

bool AbstractState::check_arc(Operator *op, AbstractState *other) {
    vector<bool> checked(g_variable_domain.size(), false);
    for (int i = 0; i < op->get_prevail().size(); ++i) {
        const Prevail &prevail = op->get_prevail()[i];
        const int &var = prevail.var;
        const int &value = prevail.prev;
        // Check if operator is applicable.
        assert(value != -1);
        if (!values[var][value])
            return false;
        // Check if we land in the desired state.
        // If this == other we have already done the check above.
        if ((this != other) && (!other->values[var][value]))
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
        if ((pre != -1) && (!values[var][pre]))
            return false;
        // Check if we land in the desired state.
        if (!other->values[var][post])
            return false;
        checked[var] = true;
    }
    if (this != other) {
        for (int var = 0; var < g_variable_domain.size(); ++var) {
            if (!checked[var] && !values[var].intersects(other->values[var]))
                return false;
        }
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
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        if (!values[i][conc_state[i]])
            return false;
    }
    return true;
}

bool AbstractState::is_abstraction_of(const AbstractState &other) const {
    // Return true if all our possible value sets are supersets of the
    // other's respective sets.
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        if (!other.values[i].is_subset_of(values[i]))
            return false;
    }
    return true;
}

bool AbstractState::is_abstraction_of_goal() const {
    assert(!g_goal.empty());
    for (int i = 0; i < g_goal.size(); ++i) {
        if (!values[g_goal[i].first][g_goal[i].second]) {
            return false;
        }
    }
    return true;
}

bool AbstractState::valid() const {
    return refined_var == UNDEFINED;
}

int AbstractState::get_refined_var() const {
    return refined_var;
}

AbstractState *AbstractState::get_child(int value) {
    if (value == refined_value)
        return right_child;
    return left_child;
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
    // Example domains: 0={0,1,2}, 1={0,1}
    // Abstract state: 0={0,1}, 1={0,1} -> 2/3
    // Abstract state: 0={0,1}, 1={1} -> 1/3
    // Abstract state: 0={0,1,2}, 1={0,1} -> 1
    double fraction = 1.0;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        const int domain_size = values[var].count();
        assert(domain_size >= 1);
        fraction *= double(domain_size) / g_variable_domain[var];
    }
    assert(fraction <= 1.0);
    assert(fraction > 0.0);
    return fraction;
}

void AbstractState::release_memory() {
    vector<Arc>().swap(next);
    vector<Arc>().swap(prev);
    vector<Operator *>().swap(loops);
    vector<Domain>().swap(values);
}
}
