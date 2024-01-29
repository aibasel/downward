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
    explicit LandmarkCostPartitioningHeuristic(const plugins::Options &opts);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
