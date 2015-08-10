#ifndef POTENTIALS_DIVERSE_POTENTIAL_HEURISTICS_H
#define POTENTIALS_DIVERSE_POTENTIAL_HEURISTICS_H

#include "potential_optimizer.h"

#include "../task_proxy.h"

#include <memory>
#include <unordered_map>
#include <vector>


namespace potentials {
// TODO: Rethink ownership, use less shared pointers, remove these abbreviations.
using Function = std::unique_ptr<PotentialFunction>;
using SamplesAndFunctions = std::unordered_map<State, Function>;

class DiversePotentialHeuristics {
    PotentialOptimizer optimizer;
    // TODO: Remove max_num_heuristics and control number of heuristics
    // with num_samples parameter?
    const int max_num_heuristics;
    const int num_samples;
    // TODO: Experiment with time limits and remove them if unneeded.
    const double max_filtering_time;
    const double max_covering_time;
    std::vector<Function> diverse_functions;

    /* Filter dead end samples and duplicates. Store potential heuristics
       for remaining samples. */
    SamplesAndFunctions filter_samples_and_compute_functions(
        const std::vector<State> &samples);

    // Remove all samples for which the function achieves maximal values.
    void filter_covered_samples(
        const PotentialFunction &chosen_function,
        SamplesAndFunctions &samples_and_functions) const;

    /* Return potential function optimized for remaining samples or a
       precomputed heuristic if the former does not cover additional samples. */
    Function find_function_and_remove_covered_samples(
        SamplesAndFunctions &samples);

    /* Iteratively try to find potential functions that achieve maximal values
       for as many samples as possible. */
    void cover_samples(SamplesAndFunctions &samples_and_functions);

public:
    explicit DiversePotentialHeuristics(const Options &opts);
    ~DiversePotentialHeuristics() = default;

    // Sample states, then cover them.
    std::vector<std::unique_ptr<PotentialFunction> > &&find_functions();
};
}
#endif
