#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include "../additive_heuristic.h"
#include "../causal_graph.h"
#include "../operator.h"
#include "../state.h"
#include "../landmarks/landmark_graph.h"

#include <boost/dynamic_bitset.hpp>

#include <limits>
#include <ostream>
#include <set>
#include <string>
#include <tr1/unordered_set>
#include <utility>
#include <vector>

namespace cegar_heuristic {
extern bool DEBUG;

const int UNDEFINED = -1;
// We cannot call the variable INFINITY, because that clashes with something in c++11.
const int INF = std::numeric_limits<int>::max();

// See additive_heuristic.h.
const int MAX_COST_VALUE = 100000000;

typedef boost::dynamic_bitset<> Bitset;
typedef std::pair<int, std::vector<int> > Split;
typedef std::vector<Split> Splits;
// TODO: Turn into struct.
typedef std::pair<int, int> Fact;

bool is_not_marked(Operator &op);

int get_pre(const Operator &op, int var);
int get_eff(const Operator &op, int var);
int get_post(const Operator &op, int var);

void get_unmet_preconditions(const Operator &op, const State &state, Splits *splits);

void get_unmet_goal_conditions(const State &state, Splits *splits);

bool goal_var(int var);

struct hash_state_id {
    size_t operator()(const StateID &id) const {
        return id.hash();
    }
};

struct hash_fact {
    size_t operator()(const Fact &fact) const {
        return fact.first * 10000 + fact.second;
    }
};

Fact get_fact(const LandmarkNode *node);

void write_landmark_graph(const LandmarkGraph &graph);
void write_causal_graph();

// Debugging.
std::string to_string(int i);
std::string to_string(Fact fact);
std::string to_string(const std::vector<int> &v);
std::string to_string(const std::set<int> &s);
std::string to_string(const std::tr1::unordered_set<int> &s);

std::ostream &operator<<(std::ostream &os, const Fact &fact);
}

#endif
