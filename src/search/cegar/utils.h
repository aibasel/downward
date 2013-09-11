#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include <string>
#include <utility>
#include <vector>

#include "../causal_graph.h"
#include "../operator.h"
#include "../state.h"
#include "../landmarks/landmark_graph.h"

#include <boost/dynamic_bitset.hpp>

namespace cegar_heuristic {
extern bool DEBUG;

const int UNDEFINED = -1;
// We cannot call the variable INFINITY, because that clashes with something in c++11.
const int INF = std::numeric_limits<int>::max();

typedef std::vector<Operator *> Operators;
typedef boost::dynamic_bitset<> Bitset;
typedef std::pair<int, std::vector<int> > Split;
typedef std::vector<Split> Splits;
typedef std::pair<int, int> Fact;

int get_pre(const Operator &op, int var);
int get_post(const Operator &op, int var);

void get_unmet_preconditions(const Operator &op, const State &state, Splits *splits);

void get_unmet_goal_conditions(const State &state, Splits *splits);

bool goal_var(int var);
bool test_cegar_goal(const State &s);

bool cheaper(Operator *op1, Operator* op2);

// Create an ordering of the variables in the causal graph. Since the CG is not
// a DAG, we use the following approximation: Put the vars with few incoming and
// many outgoing edges in front and vars with few outgoing and many incoming
// edges in the back.
void partial_ordering(const CausalGraph &causal_graph, vector<int> *order);

struct hash_state {
    size_t operator()(const State &state ) const {
        return state.hash();
    }
};

int get_fact_number(const LandmarkNode *node);
int get_fact_number(int var, int value);
pair<int, int> get_fact(const LandmarkNode *node);
void get_fact_from_number(int fact_number, int &var, int &value);
void order_facts_in_landmark_graph(vector<int> *ordering);

void write_landmark_graph();

void write_causal_graph(const CausalGraph &causal_graph);

// Debugging.
string to_string(int i);
string to_string(const std::vector<int> &v);

// Testing.
State *create_state(const std::string desc);

// Create an operator with cost 1.
// prevails have the form "var value".
// pre_posts have the form "0 var pre post" (no conditional effects).
Operator create_op(const std::string desc);
Operator create_op(const std::string name, std::vector<string> prevail,
                   std::vector<string> pre_post, int cost = 1);
}

#endif
