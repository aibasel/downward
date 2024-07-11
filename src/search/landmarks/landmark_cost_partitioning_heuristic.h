#ifndef LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H
#define LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H

#include "landmark_heuristic.h"
#include "../lp/lp_solver.h"

namespace landmarks {
class CostPartitioningAlgorithm;

enum class CostPartitioningMethod {
    OPTIMAL,
    UNIFORM,
};

class LandmarkCostPartitioningHeuristic : public LandmarkHeuristic {
    std::unique_ptr<CostPartitioningAlgorithm> cost_partitioning_algorithm;

    void check_unsupported_features(
        const std::shared_ptr<LandmarkFactory> &lm_factory);
    void set_cost_partitioning_algorithm(
        CostPartitioningMethod cost_partitioning,
        lp::LPSolverType lpsolver, bool alm);

    int get_heuristic_value(const State &ancestor_state) override;
public:
    LandmarkCostPartitioningHeuristic(
        const std::shared_ptr<LandmarkFactory> &lm_factory, bool pref,
        bool prog_goal, bool prog_gn, bool prog_r,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity,
        CostPartitioningMethod cost_partitioning, bool alm,
        lp::LPSolverType lpsolver);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
