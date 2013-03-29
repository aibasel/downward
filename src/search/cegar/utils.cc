#include "utils.h"

#include <cassert>
#include <fstream>
#include <set>
#include <sstream>
#include <vector>

#include "../globals.h"
#include "../operator.h"
#include "../state.h"

using namespace std;

namespace cegar_heuristic {
bool DEBUG = false;

Operator create_op(const string desc) {
    istringstream iss("begin_operator\n" + desc + "\nend_operator");
    return Operator(iss, false);
}

Operator create_op(const string name, vector<string> prevail, vector<string> pre_post, int cost) {
    ostringstream oss;
    // Create operator description.
    oss << name << endl << prevail.size() << endl;
    for (int i = 0; i < prevail.size(); ++i)
        oss << prevail[i] << endl;
    oss << pre_post.size() << endl;
    for (int i = 0; i < pre_post.size(); ++i)
        oss << pre_post[i] << endl;
    oss << cost;
    return create_op(oss.str());
}

State *create_state(const string desc) {
    string full_desc = "begin_state\n" + desc + "\nend_state";
    istringstream iss(full_desc);
    return new State(iss);
}

int get_pre(const Operator &op, int var) {
    for (int i = 0; i < op.get_prevail().size(); i++) {
        const Prevail &prevail = op.get_prevail()[i];
        if (prevail.var == var)
            return prevail.prev;
    }
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.pre;
    }
    return UNDEFINED;
}

int get_post(const Operator &op, int var) {
    for (int i = 0; i < op.get_prevail().size(); ++i) {
        const Prevail &prevail = op.get_prevail()[i];
        if (prevail.var == var)
            return prevail.prev;
    }
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
    }
    return UNDEFINED;
}

void get_unmet_preconditions(const Operator &op, const State &state, Splits *splits) {
    assert(splits->empty());
    for (int i = 0; i < op.get_prevail().size(); ++i) {
        const Prevail &prevail = op.get_prevail()[i];
        if (state[prevail.var] != prevail.prev) {
            vector<int> wanted;
            wanted.push_back(prevail.prev);
            splits->push_back(make_pair(prevail.var, wanted));
        }
    }
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if ((pre_post.pre != -1) && (state[pre_post.var] != pre_post.pre)) {
            vector<int> wanted;
            wanted.push_back(pre_post.pre);
            splits->push_back(make_pair(pre_post.var, wanted));
        }
    }
    assert(splits->empty() == op.is_applicable(state));
}

void get_unmet_goal_conditions(const State &state, Splits *splits) {
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first;
        int value = g_goal[i].second;
        if (state[var] != value) {
            vector<int> wanted;
            wanted.push_back(value);
            splits->push_back(make_pair(var, wanted));
        }
    }
}

bool goal_var(int var) {
    for (int i = 0; i < g_goal.size(); i++) {
        if (var == g_goal[i].first)
            return true;
    }
    return false;
}

bool test_cegar_goal(const State &state) {
    for (int i = 0; i < g_goal.size(); i++) {
        if (state[g_goal[i].first] != g_goal[i].second) {
            return false;
        }
    }
    return true;
}

void partial_ordering(const CausalGraph &causal_graph, vector<int> *order) {
    assert(order->empty());
    // Set of variables that still have to be ordered.
    set<int> vars;
    set<int>::iterator it;
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        vars.insert(vars.end(), i);
    }
    // For each variable, maintain sets of predecessor and successor variables
    // that haven't been ordered yet.
    vector<set<int> > predecessors;
    vector<set<int> > successors;
    predecessors.resize(g_variable_domain.size());
    successors.resize(g_variable_domain.size());
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        const vector<int> &pre = causal_graph.get_predecessors(var);
        for (int i = 0; i < pre.size(); ++i) {
            predecessors[var].insert(pre[i]);
        }
        const vector<int> &succ = causal_graph.get_successors(var);
        for (int i = 0; i < succ.size(); ++i) {
            successors[var].insert(succ[i]);
        }
    }
    while (!vars.empty()) {
        int min_pre = g_variable_domain.size() + 1;
        int max_succ = -1;
        int var = -1;
        for (it = vars.begin(); it != vars.end(); ++it) {
            set<int> &pre = predecessors[*it];
            set<int> &succ = successors[*it];
            assert(pre.size() <= g_variable_domain.size());
            assert(succ.size() <= g_variable_domain.size());
            if (DEBUG) {
                cout << "pre(" << *it << "): ";
                for (set<int>::iterator p = pre.begin(); p != pre.end(); ++p)
                    cout << *p << " ";
                cout << "(" << succ.size() << " succ)" << endl;
            }
            if ((pre.size() < min_pre) || ((pre.size() == min_pre) && (succ.size() > max_succ))) {
                var = *it;
                min_pre = pre.size();
                max_succ = succ.size();
            }
        }
        assert(var >= 0);
        if (DEBUG)
            cout << "Choose " << var << endl << endl;
        order->push_back(var);
        vars.erase(var);
        // For all unsorted variables, delete var from their predecessor and
        // successor lists.
        for (it = vars.begin(); it != vars.end(); ++it) {
            set<int> &pre = predecessors[*it];
            set<int>::iterator pos = find(pre.begin(), pre.end(), var);
            if (pos != pre.end())
                pre.erase(pos);
            set<int> &succ = successors[*it];
            pos = find(succ.begin(), succ.end(), var);
            if (pos != succ.end())
                succ.erase(pos);
        }
    }
    assert(order->size() == g_variable_domain.size());
}

void write_causal_graph(const CausalGraph &causal_graph) {
    ofstream dotfile("causal-graph.dot");
    if (!dotfile.is_open()) {
        cout << "dot file for causal graph could not be opened" << endl;
        exit(1);
    }
    dotfile << "digraph cg {" << endl;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        const vector<int> &successors = causal_graph.get_successors(var);
        for (int i = 0; i < successors.size(); ++i) {
            dotfile << "  " << var << " -> " << successors[i] << ";" << endl;
        }
    }
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first;
        dotfile << var << " [color=red];" << endl;
    }
    dotfile << "}" << endl;
    dotfile.close();
}

string to_string(int i) {
    stringstream out;
    out << i;
    return out.str();
}

string to_string(const vector<int> &v) {
    string sep = "";
    stringstream out;
    for (int i = 0; i < v.size(); ++i) {
        out << sep << v[i];
        sep = ",";
    }
    return out.str();
}
}
