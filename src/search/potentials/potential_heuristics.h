#ifndef POTENTIALS_POTENTIAL_HEURISTICS_H
#define POTENTIALS_POTENTIAL_HEURISTICS_H

#include "../heuristic.h"

#include <memory>
#include <vector>


namespace potentials {
class PotentialHeuristics : public Heuristic {
    std::vector<std::shared_ptr<Heuristic> > heuristics;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);

public:
    PotentialHeuristics(const Options &opts);
    ~PotentialHeuristics() = default;
};
}
#endif
