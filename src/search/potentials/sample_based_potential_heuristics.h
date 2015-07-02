#ifndef POTENTIALS_SAMPLE_BASED_POTENTIAL_HEURISTICS_H
#define POTENTIALS_SAMPLE_BASED_POTENTIAL_HEURISTICS_H

#include "potential_optimizer.h"

#include <memory>
#include <vector>

class Options;


namespace potentials {

class PotentialFunction;

class SampleBasedPotentialHeuristics {
    PotentialOptimizer optimizer;
    std::vector<std::shared_ptr<PotentialFunction> > functions;

public:
    explicit SampleBasedPotentialHeuristics(const Options &opts);
    ~SampleBasedPotentialHeuristics() = default;

    std::vector<std::shared_ptr<PotentialFunction> > get_functions() const;
};

}

#endif
