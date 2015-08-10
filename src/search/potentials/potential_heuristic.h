#ifndef POTENTIALS_POTENTIAL_HEURISTIC_H
#define POTENTIALS_POTENTIAL_HEURISTIC_H

#include "../heuristic.h"

#include <memory>


namespace potentials {
class PotentialFunction;

/*
  Use an internal potential function to evaluate a given state.

  TODO: The current code contains no special cases for dead ends. Returning
        special values for them could be useful in some cases however.
        Can we and do we want to handle dead-end states separately?
*/
class PotentialHeuristic : public Heuristic {
    std::unique_ptr<PotentialFunction> function;

protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;

public:
    explicit PotentialHeuristic(
        const Options &opts, std::unique_ptr<PotentialFunction> function);
    ~PotentialHeuristic() = default;
};
}

#endif
