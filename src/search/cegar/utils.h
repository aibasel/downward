#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include <limits>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class AdditiveHeuristic;
class FactProxy;
class GoalsProxy;
class OperatorProxy;
class State;
class TaskProxy;

namespace cegar {
extern bool DEBUG;

const int UNDEFINED = -1;
// The name INFINITY is reserved in C++11.
const int INF = std::numeric_limits<int>::max();

// See additive_heuristic.h.
const int MAX_COST_VALUE = 100000000;

using Split = std::pair<int, std::vector<int> >;
using Splits =std::vector<Split>;
using Fact = std::pair<int, int>;
using VarToValues = std::unordered_map<int, std::vector<int> >;

std::shared_ptr<AdditiveHeuristic> get_additive_heuristic(TaskProxy task);

/*
  The set of relaxed-reachable facts is the possibly-before set of facts that
  can be reached in the delete-relaxation before 'fact' is reached the first
  time plus 'fact' itself.
*/
std::unordered_set<FactProxy> get_relaxed_reachable_facts(TaskProxy task, FactProxy fact);

int get_pre(OperatorProxy op, int var_id);
int get_eff(OperatorProxy op, int var_id);
int get_post(OperatorProxy op, int var_id);

void get_unmet_preconditions(OperatorProxy op, const State &state, Splits *splits);

void get_unmet_goals(GoalsProxy goals, const State &state, Splits *splits);

void reserve_memory_padding();
void release_memory_padding();
bool memory_padding_is_reserved();

// Debugging.
std::string to_string(int i);
std::string to_string(Fact fact);
std::string to_string(const std::vector<int> &v);
std::string to_string(const std::set<int> &s);
std::string to_string(const std::unordered_set<int> &s);

std::ostream &operator<<(std::ostream &os, const Fact &fact);
}

#endif
