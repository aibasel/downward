#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include "../task_proxy.h"

#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

class AbstractTask;

namespace additive_heuristic {
class AdditiveHeuristic;
}

namespace cegar {
const int UNDEFINED_VALUE = -1;

// Positive infinity. The name "INFINITY" is taken by an ISO C99 macro.
const int INF = std::numeric_limits<int>::max();

extern std::unique_ptr<additive_heuristic::AdditiveHeuristic>
create_additive_heuristic(const std::shared_ptr<AbstractTask> &task);

/*
  The set of relaxed-reachable facts is the possibly-before set of facts that
  can be reached in the delete-relaxation before 'fact' is reached the first
  time, plus 'fact' itself.
*/
extern std::unordered_set<FactProxy> get_relaxed_possible_before(
    const TaskProxy &task, const FactProxy &fact);

extern std::vector<int> get_domain_sizes(const TaskProxy &task);
}

/*
  TODO: Our proxy classes are meant to be temporary objects and as such
  shouldn't be stored in containers. Once we find a way to avoid
  storing them in containers, we should remove this hashing function.
*/
namespace std {
template<>
struct hash<FactProxy> {
    size_t operator()(const FactProxy &fact) const {
        std::hash<FactPair> hasher;
        return hasher(fact.get_pair());
    }
};
}

#endif
