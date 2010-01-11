#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Abstraction;
class AxiomEvaluator;
class Cache;
class CausalGraph;
class DomainTransitionGraph;
class Operator;
class Axiom;
class State;
class SuccessorGenerator;
class Timer;
class FFHeuristic;
class SearchSpace;

bool test_goal(const State &state);

void read_everything(istream &in);
void dump_everything();

void check_magic(istream &in, string magic);

extern bool g_legacy_file_format;
extern bool g_use_metric;
extern int g_min_action_cost;
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
extern Cache *g_cache;
extern int g_cache_hits, g_cache_misses;
extern vector<Abstraction *> g_abstractions;

extern int g_abstraction_max_size;
extern int g_abstraction_peak_memory;

extern int g_abstraction_nr;

extern Timer g_timer;

enum {
    COMPOSE_LINEAR_CG_GOAL_LEVEL,
    COMPOSE_LINEAR_CG_GOAL_RANDOM,
    COMPOSE_LINEAR_GOAL_CG_LEVEL,
    COMPOSE_LINEAR_RANDOM,
    COMPOSE_DFP,
    MAX_COMPOSE_STRATEGY
};
extern int g_compose_strategy;

enum {
    COLLAPSE_HIGH_F_LOW_H,
    COLLAPSE_LOW_F_LOW_H,
    COLLAPSE_HIGH_F_HIGH_H,
    COLLAPSE_RANDOM,
    COLLAPSE_DFP,
    MAX_COLLAPSE_STRATEGY
};
extern int g_collapse_strategy;

extern bool g_merge_and_shrink_bound_is_for_product;

extern bool g_merge_and_shrink_simplify_labels;
extern bool g_merge_and_shrink_extra_statistics;
extern bool g_merge_and_shrink_forbid_merge_across_buckets;

extern FFHeuristic *g_ff_heur;
extern SearchSpace* g_learning_search_space;

#endif
