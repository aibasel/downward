#ifndef POTENTIALS_SAMPLE_BASED_POTENTIAL_HEURISTICS_H
#define POTENTIALS_SAMPLE_BASED_POTENTIAL_HEURISTICS_H

#include "potential_optimizer.h"

#include <memory>
#include <vector>

class Options;


namespace potentials {
class SampleBasedPotentialHeuristics {
    mutable PotentialOptimizer optimizer;
    std::vector<std::shared_ptr<Heuristic> > heuristics;

public:
    SampleBasedPotentialHeuristics(const Options &opts);
    ~SampleBasedPotentialHeuristics() = default;

    std::vector<std::shared_ptr<Heuristic> > get_heuristics() const;
};
}
#endif
