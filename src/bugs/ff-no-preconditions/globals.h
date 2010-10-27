#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class AxiomEvaluator;
class Cache;
class CausalGraph;
class DomainTransitionGraph;
class FFHeuristic;
class Operator;
class Axiom;
class State;
class SuccessorGenerator;

void read_everything(istream &in);
void dump_everything();

void check_magic(istream &in, string magic);

extern vector<string> g_variable_name;
extern vector<int> g_variable_domain;
extern vector<int> g_axiom_layers;
extern vector<int> g_default_axiom_values;

extern State *g_initial_state;
extern vector<pair<int, int> > g_goal;
extern vector<Operator> g_operators;
extern vector<Operator> g_axioms;
extern AxiomEvaluator *g_axiom_evaluator;
extern SuccessorGenerator *g_successor_generator;
extern vector<DomainTransitionGraph *> g_transition_graphs;
extern CausalGraph *g_causal_graph;
extern FFHeuristic *g_ff_heuristic;
extern Cache *g_cache;
extern int g_cache_hits, g_cache_misses;

extern bool g_multi_heuristic;
extern bool g_downward_helpful_actions;
extern bool g_ff_helpful_actions;
extern bool g_mixed_helpful_actions;

#endif
