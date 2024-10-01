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
        const std::shared_ptr<LandmarkFactory> &lm_factory,
        tasks::AxiomHandlingType axioms, bool pref,
        bool prog_goal, bool prog_gn, bool prog_r,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);

    virtual bool dead_ends_are_reliable() const override;
};

extern void add_landmark_sum_heuristic_options_to_feature(
    plugins::Feature &feature, const std::string &description);
extern std::tuple<std::shared_ptr<LandmarkFactory>, tasks::AxiomHandlingType,
                  bool, bool, bool, bool, std::shared_ptr<AbstractTask>, bool,
                  std::string, utils::Verbosity>
get_landmark_sum_heuristic_arguments_from_options(
    const plugins::Options &opts);
}

#endif
