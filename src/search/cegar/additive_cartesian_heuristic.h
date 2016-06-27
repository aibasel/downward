#ifndef CEGAR_ADDITIVE_CARTESIAN_HEURISTIC_H
#define CEGAR_ADDITIVE_CARTESIAN_HEURISTIC_H

#include "../heuristic.h"

#include <memory>
#include <vector>

namespace cegar {
class CartesianHeuristicFunction;
class SubtaskGenerator;

/*
  Store CartesianHeuristicFunctions and compute overall heuristic by
  summing all of their values.
*/
class AdditiveCartesianHeuristic : public Heuristic {
    const std::vector<CartesianHeuristicFunction> heuristic_functions;

protected:
    virtual int compute_heuristic(const GlobalState &global_state);
    int compute_heuristic(const State &state);

public:
    explicit AdditiveCartesianHeuristic(const options::Options &opts);
    virtual ~AdditiveCartesianHeuristic() = default;
};
}

#endif
