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

class PotentialHeuristics: public Heuristic {
    const bool diversify;
    const int max_num_heuristics;
    const int num_samples;
    const double max_filtering_time;
    const double max_covering_time;
    const bool debug;
    PotentialOptimizer optimizer;
    std::vector<std::shared_ptr<Heuristic> > heuristics;

    void filter_samples(
            std::vector<GlobalState> &samples,
            std::unordered_map<StateID, int> &sample_to_max_h,
            std::unordered_map<StateID, std::shared_ptr<Heuristic> > &single_heuristics);
    void cover_samples(
            std::vector<GlobalState> &samples,
            const std::unordered_map<StateID, int> &sample_to_max_h,
            const std::shared_ptr<Heuristic> heuristic,
            std::unordered_map<StateID, std::shared_ptr<Heuristic> > &single_heuristics) const;
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
