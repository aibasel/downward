#include "utils.h"

#include <cassert>
#include <fstream>
#include <set>
#include <sstream>
#include <tr1/unordered_map>
#include <vector>

#include <ext/hash_map>

#include "../globals.h"
#include "../operator.h"
#include "../option_parser.h"
#include "../state.h"
#include "../timer.h"
#include "../utilities.h"
#include "../landmarks/h_m_landmarks.h"

using namespace std;
using namespace std::tr1;

namespace cegar_heuristic {
bool DEBUG = false;
typedef unordered_map<int, unordered_set<int> > Edges;

static std::vector<int> CAUSAL_GRAPH_ORDERING_POS;

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

bool is_not_marked(Operator &op) {
    return !op.is_marked();
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

int get_eff(const Operator &op, int var) {
    for (int i = 0; i < op.get_pre_post().size(); ++i) {
        const PrePost &pre_post = op.get_pre_post()[i];
        if (pre_post.var == var)
            return pre_post.post;
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
    assert(splits->empty());
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

Fact get_fact(const LandmarkNode *node) {
    assert(node);
    assert(node->vars.size() == 1);
    int var = node->vars[0];
    int value = node->vals[0];
    return Fact(var, value);
}

string get_node_name(Fact fact) {
    stringstream out;
    out << g_fact_names[fact.first][fact.second] << " (" << fact.first << "=" << fact.second << ")";
    return out.str();
}

void write_landmark_graph(const LandmarkGraph &graph) {
    const set<LandmarkNode *> &nodes = graph.get_nodes();
    set<LandmarkNode *, LandmarkNodeComparer> nodes2(nodes.begin(), nodes.end());

    ofstream dotfile("landmark-graph.dot");
    if (!dotfile.is_open()) {
        cerr << "dot file for causal graph could not be opened" << endl;
        exit_with(EXIT_CRITICAL_ERROR);
    }
    dotfile << "digraph landmark-graph {" << endl;


    for (set<LandmarkNode *>::const_iterator it = nodes2.begin(); it
         != nodes2.end(); it++) {
        LandmarkNode *node_p = *it;
        Fact node_fact = get_fact(node_p);
        for (__gnu_cxx::hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator parent_it =
                 node_p->parents.begin(); parent_it != node_p->parents.end(); ++parent_it) {
            const LandmarkNode *parent_p = parent_it->first;
            Fact parent_fact = get_fact(parent_p);
            dotfile << "  \"" << get_node_name(parent_fact) << "\" -> "
                    << "\"" << get_node_name(node_fact) << "\";" << endl;
            //if (g_initial_state()[parent_fact.first] == parent_fact.second)
            //    dotfile << "\"" << get_node_name(parent_fact) << "\" [color=green];" << endl;
            //if (g_initial_state()[node_fact.first] == node_fact.second)
            //    dotfile << "\"" << get_node_name(node_fact) << "\" [color=green];" << endl;
        }
    }
    for (int var = 0; var < g_variable_domain.size(); var++) {
    }
    for (int i = 0; i < g_goal.size(); i++) {
        dotfile << "\"" << get_node_name(g_goal[i]) << "\" [color=red];" << endl;
    }
    dotfile << "}" << endl;
    dotfile.close();
}

void write_causal_graph(const CausalGraph &causal_graph) {
    ofstream dotfile("causal-graph.dot");
    if (!dotfile.is_open()) {
        cerr << "dot file for causal graph could not be opened" << endl;
        exit_with(EXIT_CRITICAL_ERROR);
    }
    dotfile << "digraph causal-graph {" << endl;
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

string to_string(Fact fact) {
    stringstream out;
    out << fact.first << "=" << fact.second;
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

string to_string(const set<int> &s) {
    string sep = "";
    stringstream out;
    for (set<int>::const_iterator it = s.begin(); it != s.end(); ++it) {
        out << sep << *it;
        sep = ",";
    }
    return out.str();
}

string to_string(const unordered_set<int> &s) {
    string sep = "";
    stringstream out;
    for (unordered_set<int>::const_iterator it = s.begin(); it != s.end(); ++it) {
        out << sep << *it;
        sep = ",";
    }
    return out.str();
}

ostream &operator<<(ostream &os, const Fact &fact) {
    os << fact.first << "=" << fact.second;
    return os;
}
}
