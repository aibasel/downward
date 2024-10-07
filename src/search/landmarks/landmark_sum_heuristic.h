#ifndef LANDMARKS_LANDMARK_SUM_HEURISTIC_H
#define LANDMARKS_LANDMARK_SUM_HEURISTIC_H

#include "landmark_heuristic.h"

namespace landmarks {
class LandmarkSumHeuristic : public LandmarkHeuristic {
    const bool dead_ends_reliable;

    std::vector<int> min_first_achiever_costs;
    std::vector<int> min_possible_achiever_costs;

    int get_min_cost_of_achievers(
        const std::unordered_set<int> &achievers) const;
    void compute_landmark_costs();

    int get_heuristic_value(const State &ancestor_state) override;
public:
    LandmarkSumHeuristic(
        const std::shared_ptr<LandmarkFactory> &lm_factory, bool pref,
        bool prog_goal, bool prog_gn, bool prog_r,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity, tasks::AxiomHandlingType axioms);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
