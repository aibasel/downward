#ifndef COST_SATURATION_TYPES_H
#define COST_SATURATION_TYPES_H

#include <limits>
#include <memory>
#include <vector>

namespace cost_saturation {
class Abstraction;
class CostPartitionedHeuristic;

// Positive infinity. The name "INFINITY" is taken by an ISO C99 macro.
const int INF = std::numeric_limits<int>::max();

using Abstractions = std::vector<std::unique_ptr<Abstraction>>;
using CPFunction = std::function<CostPartitionedHeuristic(
                                     const Abstractions &, const std::vector<int> &, const std::vector<int> &, bool)>;
using CPHeuristics = std::vector<CostPartitionedHeuristic>;
using Order = std::vector<int>;
}

#endif
