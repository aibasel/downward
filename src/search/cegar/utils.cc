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

bool intersection_empty(const set<int> &vals1, const set<int> &vals2) {
    set<int>::iterator i = vals1.begin();
    set<int>::iterator j = vals2.begin();
    while (true) {
        while (i != vals1.end() && *i < *j) {
            ++i;
        }
        while (j != vals2.end() && *j < *i) {
            ++j;
        }
        if (i == vals1.end() || j == vals2.end()) {
            return true;
        } else if (*i == *j) {
            return false;
        }
    }
    return true;
}

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

int get_eff(const Operator &op, int var) {
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        PrePost pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
    }
    return UNDEFINED;
}

void get_prevail_and_preconditions(const Operator &op, vector<pair<int, int> > *cond) {
    for (int i = 0; i < op.get_prevail().size(); i++) {
        const Prevail *prevail = &op.get_prevail()[i];
        cond->push_back(pair<int, int>(prevail->var, prevail->prev));
    }
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        const PrePost *pre_post = &op.get_pre_post()[i];
        cond->push_back(pair<int, int>(pre_post->var, pre_post->pre));
    }
}

int get_pre(const Operator &op, int var) {
    vector<pair<int, int> > preconditions;
    get_prevail_and_preconditions(op, &preconditions);
    for (int i = 0; i < preconditions.size(); ++i) {
        if (preconditions[i].first == var)
            return preconditions[i].second;
    }
    return UNDEFINED;
}

void get_unmet_preconditions(const Operator &op, const State &state,
                             vector<pair<int, int> > *cond) {
    assert(cond->empty());
    vector<pair<int, int> > preconditions;
    get_prevail_and_preconditions(op, &preconditions);
    for (int i = 0; i < preconditions.size(); i++) {
        int var = preconditions[i].first;
        int value = preconditions[i].second;
        if (value != -1 && state[var] != value)
            cond->push_back(pair<int, int>(var, value));
    }
    assert(cond->empty() == op.is_applicable(state));
}

void get_unmet_goal_conditions(const State &state,
                               vector<pair<int, int> > *unmet_conditions) {
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first;
        int value = g_goal[i].second;
        if (state[var] != value) {
            unmet_conditions->push_back(pair<int, int>(var, value));
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

void partial_ordering(CausalGraph &causal_graph, vector<int> *order) {
    assert(order->empty());
    set<int> vars;
    set<int>::iterator it;
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        vars.insert(vars.end(), i);
    }
    vector<set<int> > predecessors;
    predecessors.resize(g_variable_domain.size());
    for (int i = 0; i < g_variable_domain.size(); ++i) {
        const vector<int> &pre = causal_graph.get_predecessors(i);
        for (int j = 0; j < pre.size(); ++j) {
            predecessors[i].insert(j);
        }
    }
    while (!vars.empty()) {
        int min_pre = g_variable_domain.size() + 1;
        int var = -1;
        for (it = vars.begin(); it != vars.end(); ++it) {
            set<int> &pre = predecessors[*it];
            assert(pre.size() <= g_variable_domain.size());
            if (pre.size() < min_pre) {
                var = *it;
                min_pre = pre.size();
            }
        }
        assert(var >= 0);
        order->push_back(var);
        vars.erase(var);
        for (it = vars.begin(); it != vars.end(); ++it) {
            predecessors[*it].erase(var);
        }
    }
    assert(order->size() == g_variable_domain.size());
}

void write_causal_graph(CausalGraph &causal_graph) {
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
    dotfile << "}" << endl;
    dotfile.close();
}
}
