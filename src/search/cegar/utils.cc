#include "utils.h"

#include "concrete_state.h"

#include "../global_state.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../state_registry.h"
#include "../timer.h"
#include "../utilities.h"

#include "../landmarks/landmark_graph.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace std;

namespace cegar {
bool DEBUG = false;

// For values <= 50 MB the planner is often killed during the search.
// Reserving 75 MB avoids this.
static const int MEMORY_PADDING_MB = 75;

static char *cegar_memory_padding = 0;

// Save previous out-of-memory handler.
static void (*global_out_of_memory_handler)(void) = 0;

shared_ptr<StateRegistry> get_state_registry(const vector<int> &initial_state_data) {
    vector<int> original_initial_state_data;
    original_initial_state_data.swap(g_initial_state_data);
    g_initial_state_data = initial_state_data;
    shared_ptr<StateRegistry> registry = make_shared<StateRegistry>();
    original_initial_state_data.swap(g_initial_state_data);
    return registry;
}

bool is_not_marked(GlobalOperator &op) {
    return !op.is_marked();
}

int get_pre(const GlobalOperator &op, int var) {
    for (size_t i = 0; i < op.get_preconditions().size(); i++) {
        const GlobalCondition &precondition = op.get_preconditions()[i];
        if (precondition.var == var)
            return precondition.val;
    }
    return UNDEFINED;
}

int get_eff(const GlobalOperator &op, int var) {
    for (size_t i = 0; i < op.get_effects().size(); ++i) {
        const GlobalEffect &effect = op.get_effects()[i];
        if (effect.var == var)
            return effect.val;
    }
    return UNDEFINED;
}

int get_post(const GlobalOperator &op, int var) {
    for (size_t i = 0; i < op.get_effects().size(); ++i) {
        const GlobalEffect &effect = op.get_effects()[i];
        if (effect.var == var)
            return effect.val;
    }
    for (size_t i = 0; i < op.get_preconditions().size(); ++i) {
        const GlobalCondition &precondition = op.get_preconditions()[i];
        if (precondition.var == var)
            return precondition.val;
    }
    return UNDEFINED;
}

void get_unmet_preconditions(const GlobalOperator &op, const ConcreteState &state, Splits *splits) {
    assert(splits->empty());
    for (size_t i = 0; i < op.get_preconditions().size(); ++i) {
        const GlobalCondition &precondition = op.get_preconditions()[i];
        if (state[precondition.var] != precondition.val) {
            vector<int> wanted;
            wanted.push_back(precondition.val);
            splits->push_back(make_pair(precondition.var, wanted));
        }
    }
    assert(splits->empty() == is_applicable(op, state));
}

void get_unmet_goals(GoalsProxy goals, const ConcreteState &state, Splits *splits) {
    assert(splits->empty());
    for (FactProxy goal : goals) {
        int var_id = goal.get_variable().get_id();
        int value = goal.get_value();
        if (state[var_id] != value) {
            vector<int> wanted;
            wanted.push_back(value);
            splits->push_back(make_pair(var_id, wanted));
        }
    }
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
    dotfile << "digraph landmarkgraph {" << endl;


    for (const auto *node_p : nodes2) {
        Fact node_fact = get_fact(node_p);
        for (const auto &parent_pair : node_p->parents) {
            const LandmarkNode *parent_p = parent_pair.first;
            Fact parent_fact = get_fact(parent_p);
            dotfile << "  \"" << get_node_name(parent_fact) << "\" -> "
                    << "\"" << get_node_name(node_fact) << "\";" << endl;
            // Mark initial state facts green.
            if (g_initial_state()[parent_fact.first] == parent_fact.second)
                dotfile << "  \"" << get_node_name(parent_fact) << "\" [color=green];" << endl;
            if (g_initial_state()[node_fact.first] == node_fact.second)
                dotfile << "  \"" << get_node_name(node_fact) << "\" [color=green];" << endl;
        }
    }
    // Mark goal facts red.
    for (size_t i = 0; i < g_goal.size(); i++) {
        dotfile << "  \"" << get_node_name(g_goal[i]) << "\" [color=red];" << endl;
    }
    dotfile << "}" << endl;
    dotfile.close();
}

string get_variable_name(int var) {
    string name = g_fact_names[var][0];
    name = name.substr(5);
    return "\"" + name + "\"";
}

bool is_goal_var(int var) {
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (g_goal[i].first == var)
            return true;
    }
    return false;
}

void write_causal_graph() {
    ofstream dotfile("causal-graph.dot");
    if (!dotfile.is_open()) {
        cerr << "dot file for causal graph could not be opened" << endl;
        exit_with(EXIT_CRITICAL_ERROR);
    }
    dotfile << "digraph causalgraph {" << endl;
    for (size_t var = 0; var < g_variable_domain.size(); ++var) {
        const vector<int> &successors = g_causal_graph->get_successors(var);
        for (size_t i = 0; i < successors.size(); ++i) {
            if (g_initial_state()[successors[i]] == 1 && !is_goal_var(var))
                dotfile << "  " << get_variable_name(var) << " -> "
                        << get_variable_name(successors[i]) << ";" << endl;
        }
    }
    for (size_t i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first;
        dotfile << "  " << get_variable_name(var) << " [color=red];" << endl;
    }
    dotfile << "}" << endl;
    dotfile.close();
}

void continuing_out_of_memory_handler() {
    assert(cegar_memory_padding);
    delete[] cegar_memory_padding;
    cegar_memory_padding = 0;
    cout << "Failed to allocate memory for CEGAR abstraction. "
         << "Released memory padding and will stop refinement now." << endl;
    assert(global_out_of_memory_handler);
    set_new_handler(global_out_of_memory_handler);
}

void reserve_memory_padding() {
    assert(!cegar_memory_padding);
    if (DEBUG)
        cout << "Reserving " << MEMORY_PADDING_MB << " MB of memory padding." << endl;
    cegar_memory_padding = new char[MEMORY_PADDING_MB * 1024 * 1024];
    global_out_of_memory_handler = set_new_handler(continuing_out_of_memory_handler);
}

void release_memory_padding() {
    if (cegar_memory_padding) {
        delete[] cegar_memory_padding;
        cegar_memory_padding = 0;
    }
    assert(global_out_of_memory_handler);
    set_new_handler(global_out_of_memory_handler);
}

bool memory_padding_is_reserved() {
    return cegar_memory_padding != 0;
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
    for (size_t i = 0; i < v.size(); ++i) {
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
