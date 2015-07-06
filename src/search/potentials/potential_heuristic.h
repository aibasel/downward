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
    std::shared_ptr<PotentialFunction> function;

protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;

public:
    explicit PotentialHeuristic(const Options &options);
    ~PotentialHeuristic() = default;
};

std::shared_ptr<Heuristic> create_potential_heuristic(
    std::shared_ptr<PotentialFunction> function);
}

#endif
