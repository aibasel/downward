#include "./cegar_heuristic.h"

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
    //cout << full_op_desc << endl;
    istringstream iss(full_op_desc);
    Operator op = Operator(iss, false);
    return op;
}

int get_eff(Operator op, int var) {
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        PrePost pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
    }
    return -2;
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
    return -2;
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
        if (!vals.empty()) {
            oss << sep << i << "=" << int_set_to_string(vals);
            sep = ",";
        }
    }
    oss << ">";
    return oss.str();
}

bool AbstractState::operator==(AbstractState other) {
    return values == other.values;
}

bool AbstractState::operator!=(AbstractState other) {
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

AbstractState AbstractState::regress(Operator op) {
    AbstractState abs_state = AbstractState();

    for (int v = 0; v < g_variable_domain.size(); ++v) {
        set<int> s1_vals;
        // s2_vals = s2[v]
        set<int> s2_vals = get_values(v);
        // if v occurs in op.eff:
        int eff = get_eff(op, v);
        if (eff != -2) {
            // if op.eff[v] not in s2_vals:
            if (s2_vals.count(eff) == 0) {
                // return regression_empty
                // TODO
                return abs_state;
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
        if (pre != -2) {
            // if op.pre[v] not in s1_vals:
            if (s1_vals.count(pre) == 0) {
                // return regression_empty
                // TODO
                return abs_state;
            }
            // s1_vals = [op.pre[v]]
            s1_vals.clear();
            s1_vals.insert(pre);
        }
        abs_state.values[v] = s1_vals;
    }
    return abs_state;
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
        Operator op = prev[i].first;
        AbstractState u = prev[i].second;
        if (u != *this) {
            u.remove_arc(op, *this);
            u.check_arc(op, *v1);
            u.check_arc(op, *v2);
        }
    }
    for (int i = 0; i < next.size(); ++i) {
        Operator op = next[i].first;
        AbstractState w = next[i].second;
        if (w != *this) {
            // Handle former self-loops. The same loops also were in prev,
            // but they only have to be checked once.
            v1->check_arc(op, *v2);
            v2->check_arc(op, *v1);
            v1->check_arc(op, *v1);
            v2->check_arc(op, *v2);
        } else {
            w.remove_arc(op, *this);
            v1->check_arc(op, w);
            v2->check_arc(op, w);
        }
    }

}

void AbstractState::add_arc(Operator &op, AbstractState &other) {
    next.push_back(Arc(op, other));
    other.next.push_back(Arc(op, *this));
}

void AbstractState::remove_arc(Operator &op, AbstractState &other) {
    for (int i = 0; i < next.size(); ++i) {
        Operator current_op = next[i].first;
        AbstractState current_state = next[i].second;
        // TODO(jendrik): op.get_name() may not be unique.
        if ((current_op.get_name() == op.get_name()) && (current_state == other)) {
            next.erase(next.begin() + i);
            return;
        }
    }
}

bool AbstractState::check_arc(Operator &op, AbstractState &other) {
    if (!applicable(op))
        return false;
    AbstractState result;
    apply(op, &result);
    if (result.agrees_with(other)) {
        add_arc(op, other);
        return true;
    }
    return false;
}

bool AbstractState::applicable(const Operator &op) {
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

void AbstractState::apply(const Operator &op, AbstractState *result) {
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

bool AbstractState::agrees_with(const AbstractState &other) {
    // Two abstract states agree if for all variables the sets of possible
    // values sets have a non-empty intersection.
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        vector<int> both(g_variable_domain[i]);
        vector<int>::iterator it;
        set<int> vals1 = get_values(i);
        set<int> vals2 = other.get_values(i);
        it = set_intersection(vals1.begin(), vals1.end(), vals2.begin(), vals2.end(), both.begin());
        int elements = int(it - both.begin());
        if (elements == 0)
            return false;
    }
    return true;
}

CegarHeuristic::CegarHeuristic(const Options &opts)
    : Heuristic(opts) {
    min_operator_cost = numeric_limits<int>::max();
    for (int i = 0; i < g_operators.size(); ++i)
        min_operator_cost = min(min_operator_cost,
                                get_adjusted_cost(g_operators[i]));
}

CegarHeuristic::~CegarHeuristic() {
}

void CegarHeuristic::initialize() {
    cout << "Initializing cegar heuristic..." << endl;
}

int CegarHeuristic::compute_heuristic(const State &state) {
    if (test_goal(state))
        return 0;
    else
        return min_operator_cost;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new CegarHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin("cegar", _parse);

}
