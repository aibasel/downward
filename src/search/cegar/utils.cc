#include "utils.h"

#include "../global_state.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../state_registry.h"
#include "../task_tools.h"
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

int get_pre(OperatorProxy op, int var_id) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (precondition.get_variable().get_id() == var_id)
            return precondition.get_value();
    }
    return UNDEFINED;
}

int get_eff(OperatorProxy op, int var_id) {
    for (EffectProxy effect : op.get_effects()) {
        if (effect.get_fact().get_variable().get_id() == var_id)
            return effect.get_fact().get_value();
    }
    return UNDEFINED;
}

int get_post(OperatorProxy op, int var_id) {
    int eff = get_eff(op, var_id);
    if (eff != UNDEFINED)
        return eff;
    return get_pre(op, var_id);
}

void get_unmet_preconditions(OperatorProxy op, const State &state, Splits *splits) {
    assert(splits->empty());
    for (FactProxy precondition : op.get_preconditions()) {
        VariableProxy var = precondition.get_variable();
        if (state[var] != precondition) {
            vector<int> wanted;
            wanted.push_back(precondition.get_value());
            splits->push_back(make_pair(var.get_id(), wanted));
        }
    }
    assert(splits->empty() == is_applicable(op, state));
}

void get_unmet_goals(GoalsProxy goals, const State &state, Splits *splits) {
    assert(splits->empty());
    for (FactProxy goal : goals) {
        VariableProxy var = goal.get_variable();
        if (state[var] != goal) {
            vector<int> wanted;
            wanted.push_back(goal.get_value());
            splits->push_back(make_pair(var.get_id(), wanted));
        }
    }
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
