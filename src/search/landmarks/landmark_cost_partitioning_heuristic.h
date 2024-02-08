#ifndef LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H
#define LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H

#include "landmark_heuristic.h"

namespace landmarks {
class CostPartitioningAlgorithm;

enum class CostPartitioningMethod {
    OPTIMAL,
    UNIFORM,
};

class LandmarkCostPartitioningHeuristic : public LandmarkHeuristic {
    std::unique_ptr<CostPartitioningAlgorithm> cost_partitioning_algorithm;

    void check_unsupported_features(const plugins::Options &opts);
    void set_cost_partitioning_algorithm(const plugins::Options &opts);

    int get_heuristic_value(const State &ancestor_state) override;
public:
    LandmarkCostPartitioningHeuristic(
        const plugins::Options &options,
        bool use_preferred_operators,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates,
        const std::string &description,
        utils::Verbosity verbosity);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
