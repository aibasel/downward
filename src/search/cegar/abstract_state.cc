#include "./abstract_state.h"

#include "./../globals.h"
#include "./../operator.h"
#include "./../option_parser.h"
#include "./../plugin.h"
#include "./../state.h"

#include <assert.h>

#include <limits>
#include <utility>
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>
using namespace std;

namespace cegar_heuristic {

// Gtest prevents us from defining this variable in the header.
int UNDEFINED = -2;

string int_set_to_string(set<int> myset) {
    ostringstream oss;
    oss << "{";
    int j = 0;
    for (set<int>::iterator iter = myset.begin(); iter != myset.end(); ++iter) {
        oss << *iter;
        ++j;
        if (j < myset.size())
            oss << ",";
    }
    oss << "}";
    return oss.str();
}

Operator create_op(const string name, vector<string> prevail, vector<string> pre_post) {
    ostringstream oss;
    // Create operator description.
    oss << name << endl << prevail.size() << endl;
    for (int i = 0; i < prevail.size(); ++i)
        oss << prevail[i] << endl;
    oss << pre_post.size() << endl;
    for (int i = 0; i < pre_post.size(); ++i)
        oss << pre_post[i] << endl;
    oss << 1;
    return create_op(oss.str());
}

Operator create_op(const std::string desc) {
    std::string full_op_desc = "begin_operator\n" + desc + "\nend_operator";
    istringstream iss(full_op_desc);
    Operator op = Operator(iss, false);
    return op;
}

State* create_state(const std::string desc) {
    std::string full_desc = "begin_state\n" + desc + "\nend_state";
    istringstream iss(full_desc);
    return new State(iss);
}

int get_eff(Operator op, int var) {
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        PrePost pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
    }
    return UNDEFINED;
}

int get_pre(Operator op, int var) {
    for (int i = 0; i < op.get_prevail().size(); ++i) {
        Prevail prevail = op.get_prevail()[i];
        if (prevail.var == var)
            return prevail.prev;
    }
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        PrePost pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.pre;
    }
    return UNDEFINED;
}

AbstractState::AbstractState(string s) {
    assert(g_variable_domain.size() > 0);
    values.resize(g_variable_domain.size(), set<int>());

    // Construct state from string s of the form "<0={0,1}>".
    istringstream iss(s, istringstream::in);
    char next;
    int var;
    int val;
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
                iss >> val;
                values[var].insert(val);
            } else {
                iss >> var;
            }
        }
    }
}

string AbstractState::str() const {
    ostringstream oss;
    string sep = "";
    oss << "<";
    for (int i = 0; i < values.size(); ++i) {
        set<int> vals = values[i];
        if (!vals.empty() && vals.size() < g_variable_domain[i]) {
            oss << sep << i << "=" << int_set_to_string(vals);
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

bool AbstractState::operator==(AbstractState &other) const {
    return values == other.values;
}

bool AbstractState::operator!=(AbstractState &other) const {
    return !(*this == other);
}

set<int> AbstractState::get_values(int var) const {
    if (values[var].empty()) {
        set<int> vals;
        for (int i = 0; i < g_variable_domain[var]; ++i)
            vals.insert(i);
        return vals;
    } else {
        return values[var];
    }
}

void AbstractState::set_value(int var, int value) {
    values[var].clear();
    values[var].insert(value);
}

void AbstractState::regress(const Operator &op, AbstractState *result) const {
    for (int v = 0; v < g_variable_domain.size(); ++v) {
        set<int> s1_vals;
        // s2_vals = s2[v]
        set<int> s2_vals = get_values(v);
        // if v occurs in op.eff:
        int eff = get_eff(op, v);
        if (eff != UNDEFINED) {
            // if op.eff[v] not in s2_vals:
            if (s2_vals.count(eff) == 0) {
                // return regression_empty
                result->values.clear();
                return;
            }
            // s1_vals = v.all_values_in_domain
            for (int i = 0; i < g_variable_domain[v]; ++i)
                s1_vals.insert(i);
        } else {
            // s1_vals = s2_vals
            s1_vals = s2_vals;
        }
        // if v occurs in op.pre:
        int pre = get_pre(op, v);
        if (pre != UNDEFINED) {
            // if op.pre[v] not in s1_vals:
            if (s1_vals.count(pre) == 0) {
                // return regression_empty
                result->values.clear();
                return;
            }
            // s1_vals = [op.pre[v]]
            s1_vals.clear();
            s1_vals.insert(pre);
        }
        result->values[v] = s1_vals;
    }
}

void AbstractState::refine(int var, int value, AbstractState *v1, AbstractState *v2) {
    // We can only refine for vars that can have at least two values.
    assert(get_values(var).size() >= 2);
    // The desired value has to be in the set of possible values.
    assert(get_values(var).count(value) == 1);

    v1->values = values;
    v2->values = values;

    // In v1 var can have all of the previous values except the desired one.
    v1->values[var] = get_values(var);
    v1->values[var].erase(value);

    // In v2 var can only have the desired value.
    v2->set_value(var, value);

    // u -> this -> w
    //  ==>
    // u -> v1 -> v2 -> w
    for (int i = 0; i < prev.size(); ++i) {
        Operator *op = prev[i].first;
        AbstractState *u = prev[i].second;
        if (u != this) {
            assert(*u != *this);
            u->remove_next_arc(op, this);
            u->check_arc(op, v1);
            u->check_arc(op, v2);
        }
    }
    for (int i = 0; i < next.size(); ++i) {
        Operator *op = next[i].first;
        AbstractState *w = next[i].second;
        if (w == this) {
            assert(*w == *this);
            // Handle former self-loops. The same loops also were in prev,
            // but they only have to be checked once.
            v1->check_arc(op, v2);
            v2->check_arc(op, v1);
            v1->check_arc(op, v1);
            v2->check_arc(op, v2);
        } else {
            w->remove_prev_arc(op, this);
            v1->check_arc(op, w);
            v2->check_arc(op, w);
        }
    }
    // Save the refinement hierarchy.
    this->var = var;
    Domain values = v1->get_values(var);
    for (Domain::iterator it = values.begin(); it != values.end(); ++it)
        children[*it] = v1;
    assert(v2->get_values(var).size() == 1);
    children[value] = v2;
    left = v1;
    right = v2;
}

void AbstractState::add_arc(Operator *op, AbstractState *other) {
    next.push_back(Arc(op, other));
    other->prev.push_back(Arc(op, this));
}

void AbstractState::remove_arc(vector<Arc> arcs, Operator *op, AbstractState *other) {
    for (int i = 0; i < arcs.size(); ++i) {
        Operator *current_op = arcs[i].first;
        AbstractState *current_state = arcs[i].second;
        if ((current_op == op) && (current_state == other)) {
            // TODO(jendrik): Remove later, because op.get_name() may not be unique.
            assert((current_op->get_name() == op->get_name()) && (*current_state == *other));
            arcs.erase(next.begin() + i);
            return;
        }
    }
    // Do not try to remove an operator that is not there.
    cout << "REMOVE: " << str() << " " << op->get_name() << " " << other->str() << endl;
    assert(false);
}

void AbstractState::remove_next_arc(Operator *op, AbstractState *other) {
    remove_arc(next, op, other);
}

void AbstractState::remove_prev_arc(Operator *op, AbstractState *other) {
    remove_arc(prev, op, other);
}

bool AbstractState::check_arc(Operator *op, AbstractState *other) {
    if (!applicable(*op))
        return false;
    AbstractState result;
    apply(*op, &result);
    if (result.agrees_with(*other)) {
        add_arc(op, other);
        return true;
    }
    return false;
}

bool AbstractState::applicable(const Operator &op) const {
    for (int i = 0; i < op.get_prevail().size(); ++i) {
        // Check if prevail value is in the set of possible values.
        if (get_values(op.get_prevail()[i].var).count(op.get_prevail()[i].prev) == 0)
            return false;
    }
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        // Check if pre value is in the set of possible values.
        if (get_values(op.get_pre_post()[i].var).count(op.get_pre_post()[i].pre) == 0)
            return false;
    }
    return true;
}

void AbstractState::apply(const Operator &op, AbstractState *result) const {
    assert(applicable(op));
    result->values = this->values;
    // We don't copy the arcs, because we don't need them.
    for (int i = 0; i < op.get_prevail().size(); ++i) {
        Prevail prevail = op.get_prevail()[i];
        // Check if prevail value is in the set of possible values.
        result->set_value(prevail.var, prevail.prev);
    }
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        // Check if pre value is in the set of possible values.
        PrePost prepost = op.get_pre_post()[i];
        result->set_value(prepost.var, prepost.post);
    }
}

bool AbstractState::agrees_with(const AbstractState &other) const {
    // Two abstract states agree if for all variables the sets of possible
    // values sets have a non-empty intersection.
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        vector<int> both(g_variable_domain[i]);
        vector<int>::iterator it;
        set<int> vals1 = this->get_values(i);
        set<int> vals2 = other.get_values(i);
        it = set_intersection(vals1.begin(), vals1.end(),
                              vals2.begin(), vals2.end(), both.begin());
        int elements = int(it - both.begin());
        if (elements == 0)
            return false;
    }
    return true;
}

bool AbstractState::is_abstraction_of(const State &conc_state) const {
    // Return true if every concrete value is contained in the possible values.
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        if (get_values(i).count(conc_state[i]) == 0)
            return false;
    }
    return true;
}

bool AbstractState::is_abstraction_of(const AbstractState &other) const {
    // Return true if all our possible value sets are supersets of the
    // other's respective sets.
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        vector<int> diff(g_variable_domain[i]);
        vector<int>::iterator it;
        set<int> vals1 = this->get_values(i);
        set<int> vals2 = other.get_values(i);
        // If |vals2 - vals1| == 0, vals1 is a superset of vals2.
        it = set_difference(vals2.begin(), vals2.end(),
                            vals1.begin(), vals1.end(), diff.begin());
        int elements = int(it - diff.begin());
        if (elements > 0)
            return false;
    }
    return true;
}

bool AbstractState::goal_reached() const {
    assert(g_goal.size() > 0);
    for (int i = 0; i < g_goal.size(); ++i) {
        if (get_values(g_goal[i].first).count(g_goal[i].second) == 0) {
            return false;
        }
    }
    return true;
}

bool AbstractState::valid() const {
    return children.empty();
}

int AbstractState::get_var() const {
    return var;
}

AbstractState* AbstractState::get_child(int value) {
    return children[value];
}

AbstractState* AbstractState::get_left_child() const {
    return left;
}

AbstractState* AbstractState::get_right_child() const {
    return right;
}

}
