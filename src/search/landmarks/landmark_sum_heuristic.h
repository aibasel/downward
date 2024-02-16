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
    /*
      TODO: issue1082 aimed to remove the options object from constructors.
       This is not possible here because we need to wait with initializing the
       landmark factory until the task is given (e.g., cost transformation).
       Therefore, we can only extract the landmark factory from the options
       after this happened, so we allow the landmark heuristics to keep a
       (small) options object around for that purpose.
    */
    LandmarkSumHeuristic(const plugins::Options &lm_factory_option,
                         bool use_preferred_operators,
                         bool prog_goal,
                         bool prog_gn,
                         bool prog_r,
                         const std::shared_ptr<AbstractTask> &transform,
                         bool cache_estimates,
                         const std::string &description,
                         utils::Verbosity verbosity);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
