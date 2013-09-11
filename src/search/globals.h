#ifndef GLOBALS_H
#define GLOBALS_H

#include "operator_cost.h"

#include <iosfwd>
#include <string>
#include <vector>

class Axiom;
class AxiomEvaluator;
class CausalGraph;
class DomainTransitionGraph;
class LegacyCausalGraph;
class Operator;
class RandomNumberGenerator;
class State;
class SuccessorGenerator;
class Timer;

namespace cegar_heuristic {
class Abstraction;
}


bool test_goal(const State &state);
void save_plan(const std::vector<const Operator *> &plan, int iter);
int calculate_plan_cost(const std::vector<const Operator *> &plan);

void read_everything(std::istream &in);
void dump_everything();

void verify_no_axioms_no_cond_effects();

void check_magic(std::istream &in, std::string magic);

bool are_mutex(const std::pair<int, int> &a, const std::pair<int, int> &b);

extern void no_memory();


extern bool g_use_metric;
extern bool g_is_unit_cost;
extern int g_min_action_cost;
extern int g_max_action_cost;

// TODO: The following five belong into a new Variable class.
extern std::vector<std::string> g_variable_name;
extern std::vector<int> g_variable_domain;
extern std::vector<std::vector<std::string> > g_fact_names;
extern std::vector<int> g_axiom_layers;
extern std::vector<int> g_default_axiom_values;

extern State *g_initial_state;
extern std::vector<std::pair<int, int> > g_goal;

extern std::vector<Operator> g_operators;
extern std::vector<Operator> g_axioms;
extern AxiomEvaluator *g_axiom_evaluator;
extern SuccessorGenerator *g_successor_generator;
extern std::vector<DomainTransitionGraph *> g_transition_graphs;
extern CausalGraph *g_causal_graph;
extern LegacyCausalGraph *g_legacy_causal_graph;
extern Timer g_timer;
extern std::string g_plan_filename;
extern RandomNumberGenerator g_rng;

extern cegar_heuristic::Abstraction *g_cegar_abstraction;
// Ordering of the variables in the task's causal graph.
extern std::vector<int> g_causal_graph_ordering;
// Positions in the causal graph ordering, ordered by variable.
extern std::vector<int> g_causal_graph_ordering_pos;
extern int g_num_facts;
extern std::vector<int> g_fact_borders;

// Reserve some space that can be released when no memory is left.
extern int g_memory_padding_mb;

#endif
