#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include <string>
#include <utility>
#include <vector>

#include "../causal_graph.h"
#include "../operator.h"
#include "../state.h"

#include <boost/dynamic_bitset.hpp>

namespace cegar_heuristic {
extern bool DEBUG;
const bool WRITE_DOT_FILES = false;

const int UNDEFINED = -1;

typedef std::pair<int, int> Condition;
typedef boost::dynamic_bitset<> Bitset;
typedef Bitset Domain;
typedef pair<int, int> Condition;
typedef vector<Condition> Conditions;

int get_pre(const Operator &op, int var);
int get_eff(const Operator &op, int var);

void get_unmet_preconditions(const Operator &op, const State &s,
                             std::vector<pair<int, int> > *cond);

void get_unmet_goal_conditions(const State &state,
                               vector<pair<int, int> > *unmet_conditions);

bool goal_var(int var);

// Create an operator with cost 1.
// prevails have the form "var value".
// pre_posts have the form "0 var pre post" (no conditional effects).
Operator create_op(const std::string desc);
Operator create_op(const std::string name, std::vector<string> prevail,
                   std::vector<string> pre_post, int cost = 1);

State *create_state(const std::string desc);

// Create a "partial order" from the causal graph. Since the CG is not a DAG,
// we use the following approximation: Order vars with the least incoming edges
// first. In case of ties, prefer vars with the most outgoing edges.
void partial_ordering(const CausalGraph &causal_graph, vector<int> *order);

void write_causal_graph(const CausalGraph &causal_graph);

void pick_condition_for_each_var(vector<pair<int, int> > *conditions);

// Debugging.
void print_conditions(const vector<pair<int, int> > &conditions);

extern int get_memory_in_kb(const string& type);
}

#endif
