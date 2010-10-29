#include "globals.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "axioms.h"
#include "cache.h"
#include "causal_graph.h"
#include "domain_transition_graph.h"
#include "ff_heuristic.h"
#include "operator.h"
#include "state.h"
#include "successor_generator.h"

void check_magic(istream &in, string magic) {
  string word;
  in >> word;
  if(word != magic) {
    cout << "Failed to match magic word '" << magic << "'." << endl;
    cout << "Got '" << word << "'." << endl;
    exit(1);
  }
}

void read_variables(istream &in) {
  check_magic(in, "begin_variables");
  int count;
  in >> count;
  for(int i = 0; i < count; i++) {
    string name;
    in >> name;
    g_variable_name.push_back(name);
    int range;
    in >> range;
    g_variable_domain.push_back(range);
    int layer;
    in >> layer;
    g_axiom_layers.push_back(layer);
  }
  check_magic(in, "end_variables");
}

void read_goal(istream &in) {
  check_magic(in, "begin_goal");
  int count;
  in >> count;
  for(int i = 0; i < count; i++) {
    int var, val;
    in >> var >> val;
    g_goal.push_back(make_pair(var, val));
  }
  check_magic(in, "end_goal");
}

void dump_goal() {
  cout << "Goal Conditions:" << endl;
  for(int i = 0; i < g_goal.size(); i++)
    cout << "  " << g_variable_name[g_goal[i].first] << ": "
	 << g_goal[i].second << endl;
}

void read_operators(istream &in) {
  int count;
  in >> count;
  for(int i = 0; i < count; i++)
    g_operators.push_back(Operator(in, false));
}

void read_axioms(istream &in) {
  int count;
  in >> count;
  for(int i = 0; i < count; i++)
    // g_axioms.push_back(Axiom(in));
    g_axioms.push_back(Operator(in, true));

  g_axiom_evaluator = new AxiomEvaluator;
  g_axiom_evaluator->evaluate(*g_initial_state);
}

void read_everything(istream &in) {
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
  if(g_ff_helpful_actions || g_mixed_helpful_actions || g_multi_heuristic)
    g_ff_heuristic = new FFHeuristic;
  else
    g_ff_heuristic = 0;
  g_cache = new Cache;
}

void dump_everything() {
  cout << "Variables (" << g_variable_name.size() << "):" << endl;
  for(int i = 0; i < g_variable_name.size(); i++)
    cout << "  " << g_variable_name[i]
	 << " (range " << g_variable_domain[i] << ")" << endl;
  cout << "Initial State:" << endl;
  g_initial_state->dump();
  dump_goal();
  cout << "Successor Generator:" << endl;
  g_successor_generator->dump();
  for(int i = 0; i < g_variable_domain.size(); i++)
    g_transition_graphs[i]->dump();
}

vector<string> g_variable_name;
vector<int> g_variable_domain;
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
FFHeuristic *g_ff_heuristic;
Cache *g_cache;
int g_cache_hits = 0, g_cache_misses = 0;

bool g_multi_heuristic = false;
bool g_downward_helpful_actions = false;
bool g_ff_helpful_actions = false;
bool g_mixed_helpful_actions = false;
bool g_ff_heuristic_replanning = false;

