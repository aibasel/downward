#ifndef POTENTIALS_POTENTIAL_MAX_HEURISTIC_H
#define POTENTIALS_POTENTIAL_MAX_HEURISTIC_H

#include "../heuristic.h"

#include <memory>
#include <vector>


namespace potentials {
class PotentialFunction;

/*
  Maximize over multiple potential functions.
*/
class PotentialMaxHeuristic : public Heuristic {
    std::vector<std::unique_ptr<PotentialFunction> > functions;

protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;

public:
    explicit PotentialMaxHeuristic(
        const Options &opts,
        std::vector<std::unique_ptr<PotentialFunction> > && functions);
    ~PotentialMaxHeuristic() = default;
};
}

#endif
