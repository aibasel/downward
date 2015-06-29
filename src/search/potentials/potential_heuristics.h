#ifndef POTENTIALS_POTENTIAL_HEURISTICS_H
#define POTENTIALS_POTENTIAL_HEURISTICS_H

#include "potential_optimizer.h"

#include "../global_state.h"
#include "../heuristic.h"
#include "../option_parser.h"

#include <memory>
#include <unordered_map>
#include <vector>


namespace potentials {
// TODO: Split off into two classes. One with and one without diversification.
class PotentialHeuristics : public Heuristic {
    const bool diversify;
    const int max_num_heuristics;
    const int num_samples;
    const double max_filtering_time;
    const double max_covering_time;
    mutable PotentialOptimizer optimizer;
    std::vector<std::shared_ptr<Heuristic> > heuristics;

    /* Filter dead end samples and duplicates. Store potential heuristics and
       maximum heuristic values for remaining samples. */
    void filter_dead_ends_and_duplicates(
        std::vector<GlobalState> &samples,
        std::unordered_map<StateID, int> &sample_to_max_h,
        std::unordered_map<StateID, std::shared_ptr<Heuristic> > &single_heuristics) const;

    // Remove all samples for which the heuristic achieves maximal values.
    void filter_covered_samples(
        const std::shared_ptr<Heuristic> heuristic,
        std::vector<GlobalState> &samples,
        std::unordered_map<StateID, int> &sample_to_max_h,
        std::unordered_map<StateID, std::shared_ptr<Heuristic> > &single_heuristics) const;

    /* Return potential heuristic optimized for remaining samples or a
       precomputed heuristic if the former does not cover additional samples. */
    std::shared_ptr<Heuristic> find_heuristic_and_remove_covered_samples(
        std::vector<GlobalState> &samples,
        std::unordered_map<StateID, int> &sample_to_max_h,
        std::unordered_map<StateID, std::shared_ptr<Heuristic> > &single_heuristics) const;

    /* Iteratively try to find potential heuristics that achieve maximal values
       for as many samples as possible. */
    void cover_samples(
        std::vector<GlobalState> &samples,
        std::unordered_map<StateID, int> &sample_to_max_h,
        std::unordered_map<StateID, std::shared_ptr<Heuristic> > &single_heuristics);

    // Sample states, then cover them.
    void find_diverse_heuristics();

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);

public:
    PotentialHeuristics(const Options &opts);
    ~PotentialHeuristics() = default;
};
}
#endif
