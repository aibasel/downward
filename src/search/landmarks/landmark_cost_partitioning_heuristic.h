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

    void check_unsupported_features(const plugins::Options &lm_factory_option); // TODO issue1082 this needs Options to construct the lm_factory later.
    void set_cost_partitioning_algorithm(CostPartitioningMethod cost_partitioning,
                                         lp::LPSolverType lpsolver,
                                         bool alm);

    int get_heuristic_value(const State &ancestor_state) override;
public:
    /*
      TODO: issue1082 aimed to remove the options object from constructors.
       This is not possible here because we need to wait with initializing the
       landmark factory until the task is given (e.g., cost transformation).
       Therefore, we can only extract the landmark factory from the options
       after this happened, so we allow the landmark heuristics to keep a
       (small) options object around for that purpose.
       This should be handled by issue559 eventually.
    */
    LandmarkCostPartitioningHeuristic(
        const plugins::Options &lm_factory_option,
        bool use_preferred_operators,
        bool prog_goal,
        bool prog_gn,
        bool prog_r,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates,
        const std::string &description,
        utils::Verbosity verbosity,
        CostPartitioningMethod cost_partitioning,
        bool alm,
        lp::LPSolverType lpsolver);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
