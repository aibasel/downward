#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>
#include <string>
#include <vector>
#include "operator_cost.h"

using namespace std;


class AxiomEvaluator;
class CausalGraph;
class DomainTransitionGraph;
class Operator;
class Axiom;
class State;
class SuccessorGenerator;
class Timer;
class HeuristicOptions;

bool test_goal(const State &state);
void save_plan(const vector<const Operator *> &plan, int iter);
int calculate_plan_cost(const vector<const Operator *> &plan);

void read_everything(istream &in);
void dump_everything();

void check_magic(istream &in, string magic);

extern bool g_legacy_file_format;
extern bool g_use_metric;
extern int g_min_action_cost;
extern int g_max_action_cost;
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
extern HeuristicOptions g_default_heuristic_options;
extern Timer g_timer;
extern string g_plan_filename;

#endif
