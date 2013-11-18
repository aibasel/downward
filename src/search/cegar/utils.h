#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "../additive_heuristic.h"
#include "../causal_graph.h"
#include "../operator.h"
#include "../state.h"
#include "../landmarks/landmark_graph.h"

#include <boost/dynamic_bitset.hpp>

#include <tr1/unordered_set>

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

bool is_not_marked(Operator &op);

int get_pre(const Operator &op, int var);
int get_eff(const Operator &op, int var);
int get_post(const Operator &op, int var);

void get_unmet_preconditions(const Operator &op, const State &state, Splits *splits);

void get_unmet_goal_conditions(const State &state, Splits *splits);

bool goal_var(int var);
bool test_cegar_goal(const State &s);

struct hash_state {
    size_t operator()(const State &state) const {
        return state.hash();
    }
};

struct hash_fact {
    size_t operator()(const Fact &fact) const {
        return fact.first * 10000 + fact.second;
    }
};

Fact get_fact(const LandmarkNode *node);

void write_landmark_graph(const LandmarkGraph &graph);
void write_causal_graph(const CausalGraph &causal_graph);

// Debugging.
string to_string(int i);
string to_string(Fact fact);
string to_string(const std::vector<int> &v);
string to_string(const std::set<int> &s);
string to_string(const std::tr1::unordered_set<int> &s);

std::ostream &operator<<(std::ostream &os, const Fact &fact);

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
