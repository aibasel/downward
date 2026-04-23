#ifndef CARTESIAN_ABSTRACTIONS_CARTESIAN_HEURISTIC_FUNCTION_H
#define CARTESIAN_ABSTRACTIONS_CARTESIAN_HEURISTIC_FUNCTION_H

#include <memory>
#include <vector>

class State;

namespace cartesian_abstractions {
class RefinementHierarchy;
/*
  Store RefinementHierarchy and heuristic values for looking up abstract state
  IDs and corresponding heuristic values efficiently.
*/
class CartesianHeuristicFunction {
    // Avoid const to enable moving.
    std::unique_ptr<RefinementHierarchy> refinement_hierarchy;
    std::vector<int> h_values;

public:
    CartesianHeuristicFunction(
        std::unique_ptr<RefinementHierarchy> &&hierarchy,
        std::vector<int> &&h_values);

    CartesianHeuristicFunction(const CartesianHeuristicFunction &) = delete;
    CartesianHeuristicFunction(CartesianHeuristicFunction &&) = default;

    int get_value(const State &state) const;
};
}

#endif
