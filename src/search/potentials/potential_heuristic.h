#ifndef POTENTIALS_POTENTIAL_HEURISTIC_H
#define POTENTIALS_POTENTIAL_HEURISTIC_H

#include "../heuristic.h"

#include <vector>

namespace potentials {
class PotentialHeuristic : public Heuristic {
    std::vector<std::vector<double> > fact_potentials;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);

public:
    PotentialHeuristic(const Options &options,
                       const std::vector<std::vector<double> > &fact_potentials);
    ~PotentialHeuristic() = default;
};
}
#endif
