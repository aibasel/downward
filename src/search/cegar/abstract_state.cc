#include "abstract_state.h"

#include "values.h"

#include "../globals.h"
#include "../operator.h"
#include "../state.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <utility>
#include <vector>

using namespace std;

namespace cegar_heuristic {
AbstractState::AbstractState(string s)
    : values(new Values()),
      distance(UNDEFINED),
      prev_solution_op(0),
      next_solution_op(0),
      prev_solution_state(0),
      next_solution_state(0),
      node(0) {
    assert(!g_variable_domain.empty());

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

bool AbstractState::domains_intersect(const AbstractState *other, int var) {
    return values->domains_intersect(*other->values, var);
}

int AbstractState::count(int var) const {
    return values->count(var);
}

void AbstractState::update_incoming_arcs(int var, AbstractState *v1, AbstractState *v2) {
    for (Arcs::iterator it = arcs_in.begin(); it != arcs_in.end(); ++it) {
        const Operator *op = it->first;
        AbstractState *u = it->second;
        assert(u != this);
        int post = get_post(*op, var);
        if (post == UNDEFINED) {
            // If the domains of u and v1 don't intersect, we must add the other arc.
            bool u_and_v1_intersect = u->domains_intersect(v1, var);
            if (u_and_v1_intersect) {
                u->add_arc(op, v1);
            }
            if (!u_and_v1_intersect || u->domains_intersect(v2, var)) {
                u->add_arc(op, v2);
            }
        } else if (v2->values->test(var, post)) {
            u->add_arc(op, v2);
        } else {
            u->add_arc(op, v1);
        }
        u->remove_next_arc(op, this);
    }
}

void AbstractState::update_outgoing_arcs(int var, AbstractState *v1, AbstractState *v2) {
    for (Arcs::iterator it = arcs_out.begin(); it != arcs_out.end(); ++it) {
        const Operator *op = it->first;
        AbstractState *w = it->second;
        assert(w != this);
        int pre = get_pre(*op, var);
        int post = get_post(*op, var);
        if (post == UNDEFINED) {
            // If the domains of v1 and w don't intersect, we must add the other arc.
            bool v1_and_w_intersect = v1->domains_intersect(w, var);
            if (v1_and_w_intersect) {
                v1->add_arc(op, w);
            }
            if (!v1_and_w_intersect || v2->domains_intersect(w, var)) {
                v2->add_arc(op, w);
            }
        } else if (pre == UNDEFINED) {
            v1->add_arc(op, w);
            v2->add_arc(op, w);
        } else if (v2->values->test(var, pre)) {
            v2->add_arc(op, w);
        } else {
            v1->add_arc(op, w);
        }
        w->remove_prev_arc(op, this);
    }
}

void AbstractState::update_loops(int var, AbstractState *v1, AbstractState *v2) {
    for (int i = 0; i < loops.size(); ++i) {
        const Operator *op = loops[i];
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
    assert(v1->count(var) == count(var) - wanted.size());
    assert(v2->count(var) == wanted.size());

    // Check that the sets of possible values are now smaller.
    assert(this->is_abstraction_of(*v1));
    assert(this->is_abstraction_of(*v2));

    // Update transition system.
    update_incoming_arcs(var, v1, v2);
    update_outgoing_arcs(var, v1, v2);
    update_loops(var, v1, v2);

    // Update split tree.
    node->split(var, wanted, v1, v2);

    // Since h-values only increase we can assign the h-value to the children.
    int h = node->get_h();
    v1->set_h(h);
    v2->set_h(h);
}

void AbstractState::add_arc(const Operator *op, AbstractState *other) {
    // Experiments showed that keeping the arcs sorted for faster removal
    // increases the overall processing time. Out of 30 domains it made no
    // difference for 10 domains, 17 domains preferred unsorted arcs and in
    // 3 domains performance was better with sorted arcs.
    // Inlining this method has no effect either.
    assert(other != this);
    arcs_out.push_back(Arc(op, other));
    other->arcs_in.push_back(Arc(op, this));
}

void AbstractState::add_loop(const Operator *op) {
    loops.push_back(op);
}

void AbstractState::remove_arc(Arcs &arcs, const Operator *op, AbstractState *other) {
    Arcs::iterator pos = find(arcs.begin(), arcs.end(), Arc(op, other));
    assert(pos != arcs.end());
    // For PODs assignment is faster than swapping.
    *pos = arcs.back();
    arcs.pop_back();
}

void AbstractState::remove_next_arc(const Operator *op, AbstractState *other) {
    remove_arc(arcs_out, op, other);
}

void AbstractState::remove_prev_arc(const Operator *op, AbstractState *other) {
    remove_arc(arcs_in, op, other);
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

double AbstractState::get_rel_conc_states() const {
    double fraction = 1.0;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        const double domain_size = values->count(var);
        assert(domain_size >= 1);
        fraction *= domain_size / g_variable_domain[var];
    }
    assert(fraction <= 1.0);
    assert(fraction > 0.0);
    return fraction;
}
}
