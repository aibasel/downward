#ifndef POTENTIALS_POTENTIAL_HEURISTIC_H
#define POTENTIALS_POTENTIAL_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

namespace potentials {
class PotentialFunction;

/*
  Use an internal potential function to evaluate a given state.
*/
class PotentialHeuristic : public Heuristic {
    std::unique_ptr<PotentialFunction> function;

protected:
    virtual int compute_heuristic(const State &ancestor_state) override;

public:
    explicit PotentialHeuristic(
        std::unique_ptr<PotentialFunction> function,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);
};
}

#endif
