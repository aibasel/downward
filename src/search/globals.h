#ifndef GLOBALS_H
#define GLOBALS_H

#include "operator_cost.h"
// TODO: Get rid of the dependency on utilities.h. This is only needed
//       for hash_int_pair in g_pddl_propositions below, and the precise
//       type of that should be hidden away somewhere.
#include "utilities.h"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <ext/hash_map>

class AxiomEvaluator;
class CausalGraph;
class DomainTransitionGraph;
class Operator;
class Axiom;
class State;
class SuccessorGenerator;
class Timer;
class RandomNumberGenerator;

bool test_goal(const State &state);
void save_plan(const std::vector<const Operator *> &plan, int iter);
int calculate_plan_cost(const std::vector<const Operator *> &plan);

void read_everything(std::istream &in);
void dump_everything();

void verify_no_axioms_no_cond_effects();

void check_magic(std::istream &in, std::string magic);

bool are_mutex(const std::pair<int, int> &a, const std::pair<int, int> &b);


struct PddlProposition {
    std::string predicate;
    std::vector<std::string> arguments;
    std::string to_string() const {
        std::string output = predicate;
        for (size_t i = 0; i < arguments.size(); ++i) {
            output += " ";
            output += arguments[i];
        }
        return output;
    }
};


extern bool g_use_metric;
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
// TODO: The following three come out of the landmark code and need to
//       be cleaned up.
// TODO: g_inconsistent_facts needs a better name (g_mutexes?) and type
//       (something sane, or at least an explanation). And it doesn't
//       actually need to be globally visible, since it is only accessed
//       via the are_mutex(...) global function.
//       => Turn this into a Mutexes class with with a mutex() method.
extern std::vector<std::vector<std::set<std::pair<int, int> > > > g_inconsistent_facts;
extern __gnu_cxx::hash_map<std::pair<int, int>, PddlProposition, hash_int_pair> g_pddl_propositions;
extern std::map<std::string, int> g_pddl_proposition_indices; //TODO: make this a hash_map

extern std::vector<Operator> g_operators;
extern std::vector<Operator> g_axioms;
extern AxiomEvaluator *g_axiom_evaluator;
extern SuccessorGenerator *g_successor_generator;
extern std::vector<DomainTransitionGraph *> g_transition_graphs;
extern CausalGraph *g_causal_graph;
extern Timer g_timer;
extern std::string g_plan_filename;
extern RandomNumberGenerator g_rng;

#endif
