#ifndef POTENTIALS_DIVERSE_POTENTIAL_HEURISTICS_H
#define POTENTIALS_DIVERSE_POTENTIAL_HEURISTICS_H

#include "potential_optimizer.h"

#include "../state_id.h"

#include <memory>
#include <unordered_map>
#include <vector>


namespace potentials {
class DiversePotentialHeuristics {
    PotentialOptimizer optimizer;
    const int max_num_heuristics;
    const int num_samples;
    const double max_filtering_time;
    const double max_covering_time;
    std::unordered_map<StateID, std::shared_ptr<PotentialFunction> > single_functions;
    std::unordered_map<StateID, int> sample_to_max_h;
    std::vector<std::shared_ptr<PotentialFunction> > diverse_functions;

    /* Filter dead end samples and duplicates. Store potential heuristics and
       maximum heuristic values for remaining samples. */
    void filter_dead_ends_and_duplicates(std::vector<GlobalState> &samples);

    // Remove all samples for which the heuristic achieves maximal values.
    void filter_covered_samples(
        const std::shared_ptr<PotentialFunction> heuristic,
        std::vector<GlobalState> &samples);

    /* Return potential heuristic optimized for remaining samples or a
       precomputed heuristic if the former does not cover additional samples. */
    std::shared_ptr<PotentialFunction> find_function_and_remove_covered_samples(
        std::vector<GlobalState> &samples);

    /* Iteratively try to find potential heuristics that achieve maximal values
       for as many samples as possible. */
    void cover_samples(std::vector<GlobalState> &samples);

    // Sample states, then cover them.
    void find_diverse_heuristics();

public:
    explicit DiversePotentialHeuristics(const Options &opts);
    ~DiversePotentialHeuristics() = default;

    std::vector<std::shared_ptr<PotentialFunction> > get_functions() const;
};
}
#endif
