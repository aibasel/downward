#ifndef LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H
#define LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H

#include "landmark_heuristic.h"

namespace landmarks {
class LandmarkCostAssignment;

enum class CostPartitioningStrategy {
    OPTIMAL,
    UNIFORM,
};

class LandmarkCostPartitioningHeuristic : public LandmarkHeuristic {
    const CostPartitioningStrategy cost_partitioning_strategy;
    std::unique_ptr<LandmarkCostAssignment> lm_cost_assignment;

    void check_unsupported_features(const plugins::Options &opts);
    void set_cost_assignment(const plugins::Options &opts);

    int get_heuristic_value(const State &ancestor_state) override;
public:
    explicit LandmarkCostPartitioningHeuristic(const plugins::Options &opts);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
