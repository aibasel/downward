#ifndef POTENTIALS_POTENTIAL_HEURISTIC_H
#define POTENTIALS_POTENTIAL_HEURISTIC_H

#include "../heuristic.h"

#include <vector>

using FactPotentials = std::vector<std::vector<double> >;

namespace potentials {
class PotentialHeuristic : public Heuristic {
    FactPotentials fact_potentials;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);

public:
    PotentialHeuristic(const Options &options,
                       const FactPotentials &fact_potentials);
    ~PotentialHeuristic() = default;
};
}
#endif
