#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include "../task_proxy.h"

#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

class AbstractTask;

namespace AdditiveHeuristic {
class AdditiveHeuristic;
}

namespace CEGAR {
extern bool DEBUG;

const int UNDEFINED = -1;
// Positive infinity. The name "INFINITY" is taken by an ISO C99 macro.
const int INF = std::numeric_limits<int>::max();

// See additive_heuristic.h.
const int MAX_COST_VALUE = 100000000;

using Fact = std::pair<int, int>;

extern std::unique_ptr<AdditiveHeuristic::AdditiveHeuristic> get_additive_heuristic(
    const std::shared_ptr<AbstractTask> &task);

/*
  The set of relaxed-reachable facts is the possibly-before set of facts that
  can be reached in the delete-relaxation before 'fact' is reached the first
  time, plus 'fact' itself.
*/
extern std::unordered_set<FactProxy> get_relaxed_reachable_facts(
    TaskProxy task, FactProxy fact);

extern std::vector<int> get_domain_sizes(TaskProxy task);

extern int get_pre(OperatorProxy op, int var_id);
extern int get_eff(OperatorProxy op, int var_id);
extern int get_post(OperatorProxy op, int var_id);
}

namespace std {
template<>
struct hash<FactProxy> {
    size_t operator()(const FactProxy &fact) const {
        std::pair<int, int> raw_fact = make_pair(
            fact.get_variable().get_id(), fact.get_value());
        std::hash<std::pair<int, int>> hasher;
        return hasher(raw_fact);
    }
};
}

#endif
