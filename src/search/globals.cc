#include "globals.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
#include <sstream>
using namespace std;

#include "axioms.h"
#include "causal_graph.h"
#include "domain_transition_graph.h"
#include "heuristic.h"
#include "operator.h"
#include "rng.h"
#include "state.h"
#include "successor_generator.h"
#include "timer.h"


static const int PRE_FILE_VERSION = 2;


bool test_goal(const State &state) {
    for (int i = 0; i < g_goal.size(); i++) {
        if (state[g_goal[i].first] != g_goal[i].second) {
            return false;
        }
    }
    return true;
}

int calculate_plan_cost(const vector<const Operator *> &plan) {
    // TODO: Refactor: this is only used by save_plan (see below)
    //       and the SearchEngine classes and hence should maybe
    //       be moved into the SearchEngine (along with save_plan).
    int plan_cost = 0;
    for (int i = 0; i < plan.size(); i++) {
        plan_cost += plan[i]->get_cost();
    }
    return plan_cost;
}

void save_plan(const vector<const Operator *> &plan, int iter) {
    // TODO: Refactor: this is only used by the SearchEngine classes
    //       and hence should maybe be moved into the SearchEngine.
    ofstream outfile;
    if (iter == 0) {
        outfile.open(g_plan_filename.c_str(), ios::out);
    } else {
        ostringstream out;
        out << g_plan_filename << "." << iter;
        outfile.open(out.str().c_str(), ios::out);
    }
    for (int i = 0; i < plan.size(); i++) {
        cout << plan[i]->get_name() << " (" << plan[i]->get_cost() << ")" << endl;
        outfile << "(" << plan[i]->get_name() << ")" << endl;
    }
    outfile.close();
    int plan_cost = calculate_plan_cost(plan);
    ofstream statusfile;
    statusfile.open("plan_numbers_and_cost", ios::out | ios::app);
    statusfile << iter << " " << plan_cost << endl;
    statusfile.close();
    cout << "Plan length: " << plan.size() << " step(s)." << endl;
    cout << "Plan cost: " << plan_cost << endl;
}

bool peek_magic(istream &in, string magic) {
    string word;
    in >> word;
    bool result = (word == magic);
    for (int i = word.size() - 1; i >= 0; i--)
        in.putback(word[i]);
    return result;
}

void check_magic(istream &in, string magic) {
    string word;
    in >> word;
    if (word != magic) {
        cout << "Failed to match magic word '" << magic << "'." << endl;
        cout << "Got '" << word << "'." << endl;
        if (magic == "begin_version") {
            cerr << "Possible cause: you are running the planner "
                 << "on a preprocessor file from " << endl
                 << "an older version." << endl;
        }
        exit(1);
    }
}

void read_and_verify_version(istream &in) {
    int version;
    check_magic(in, "begin_version");
    in >> version;
    check_magic(in, "end_version");
    if (version != PRE_FILE_VERSION) {
        cerr << "Expected preprocessor file version " << PRE_FILE_VERSION
             << ", got " << version << "." << endl;
        cerr << "Exiting." << endl;
        exit(1);
    }
}

void read_metric(istream &in) {
    check_magic(in, "begin_metric");
    in >> g_use_metric;
    check_magic(in, "end_metric");
}

void read_variables(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; i++) {
        check_magic(in, "begin_variable");
        string name;
        in >> name;
        g_variable_name.push_back(name);
        int layer;
        in >> layer;
        g_axiom_layers.push_back(layer);
        int range;
        in >> range;
        g_variable_domain.push_back(range);
        if (range > numeric_limits<state_var_t>::max()) {
            cerr << "This should not have happened!" << endl;
            cerr << "Are you using the downward script, or are you using "
                 << "downward-1 directly?" << endl;
            exit(1);
        }

        in >> ws;
        vector<string> fact_names(range);
        for (size_t i = 0; i < fact_names.size(); i++)
            getline(in, fact_names[i]);
        g_fact_names.push_back(fact_names);
        check_magic(in, "end_variable");
    }
}

void read_goal(istream &in) {
    check_magic(in, "begin_goal");
    int count;
    in >> count;
    for (int i = 0; i < count; i++) {
        int var, val;
        in >> var >> val;
        g_goal.push_back(make_pair(var, val));
    }
    check_magic(in, "end_goal");
}

void dump_goal() {
    cout << "Goal Conditions:" << endl;
    for (int i = 0; i < g_goal.size(); i++)
        cout << "  " << g_variable_name[g_goal[i].first] << ": "
             << g_goal[i].second << endl;
}

void read_operators(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; i++)
        g_operators.push_back(Operator(in, false));
}

void read_axioms(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; i++)
        g_axioms.push_back(Operator(in, true));

    g_axiom_evaluator = new AxiomEvaluator;
    g_axiom_evaluator->evaluate(*g_initial_state);
}

void read_everything(istream &in) {
    read_and_verify_version(in);
    read_metric(in);
    read_variables(in);
    g_initial_state = new State(in);
    read_goal(in);
    read_operators(in);
    read_axioms(in);
    check_magic(in, "begin_SG");
    g_successor_generator = read_successor_generator(in);
    check_magic(in, "end_SG");
    DomainTransitionGraph::read_all(in);
    g_causal_graph = new CausalGraph(in);
}

void dump_everything() {
    cout << "Use metric? " << g_use_metric << endl;
    cout << "Min Action Cost: " << g_min_action_cost << endl;
    cout << "Max Action Cost: " << g_max_action_cost << endl;
    // TODO: Dump the actual fact names.
    cout << "Variables (" << g_variable_name.size() << "):" << endl;
    for (int i = 0; i < g_variable_name.size(); i++)
        cout << "  " << g_variable_name[i]
             << " (range " << g_variable_domain[i] << ")" << endl;
    cout << "Initial State:" << endl;
    g_initial_state->dump();
    dump_goal();
    /*
    cout << "Successor Generator:" << endl;
    g_successor_generator->dump();
    for(int i = 0; i < g_variable_domain.size(); i++)
      g_transition_graphs[i]->dump();
    */
}

void verify_no_axioms_no_cond_effects() {
    if (!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl << "Terminating."
             << endl;
        exit(1);
    }

    for (int i = 0; i < g_operators.size(); i++) {
        const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
        for (int j = 0; j < pre_post.size(); j++) {
            const vector<Prevail> &cond = pre_post[j].cond;
            if (cond.empty())
                continue;
            // Accept conditions that are redundant, but nothing else.
            // In a better world, these would never be included in the
            // input in the first place.
            int var = pre_post[j].var;
            int pre = pre_post[j].pre;
            int post = pre_post[j].post;
            if (pre == -1 && cond.size() == 1 && cond[0].var == var
                && cond[0].prev != post && g_variable_domain[var] == 2)
                continue;

            cerr << "Heuristic does not support conditional effects "
                 << "(operator " << g_operators[i].get_name() << ")" << endl
                 << "Terminating." << endl;
            exit(1);
        }
    }
}

bool g_legacy_file_format = false; // TODO: Can rip this out after migration.
bool g_use_metric;
int g_min_action_cost = numeric_limits<int>::max();
int g_max_action_cost = 0;
vector<string> g_variable_name;
vector<int> g_variable_domain;
vector<vector<string> > g_fact_names;
vector<int> g_axiom_layers;
vector<int> g_default_axiom_values;
State *g_initial_state;
vector<pair<int, int> > g_goal;
vector<Operator> g_operators;
vector<Operator> g_axioms;
AxiomEvaluator *g_axiom_evaluator;
SuccessorGenerator *g_successor_generator;
vector<DomainTransitionGraph *> g_transition_graphs;
CausalGraph *g_causal_graph;

Timer g_timer;
string g_plan_filename = "sas_plan";
RandomNumberGenerator g_rng(2011); // Use an arbitrary default seed.
