#ifndef CEGAR_ADDITIVE_CARTESIAN_HEURISTIC_H
#define CEGAR_ADDITIVE_CARTESIAN_HEURISTIC_H

#include "../heuristic.h"

#include <memory>
#include <vector>

namespace cegar {
class CartesianHeuristicFunction;
class SubtaskGenerator;

class AdditiveCartesianHeuristic : public Heuristic {
    std::vector<std::unique_ptr<CartesianHeuristicFunction>> heuristic_functions;

protected:
    virtual int compute_heuristic(const GlobalState &global_state);
    int compute_heuristic(const State &state);

public:
    AdditiveCartesianHeuristic(
        const options::Options &opts,
        std::vector<std::unique_ptr<CartesianHeuristicFunction>> &&heuristic_functions);
    virtual ~AdditiveCartesianHeuristic() = default;
};
}

#endif
