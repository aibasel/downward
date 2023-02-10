#ifndef LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H
#define LANDMARKS_LANDMARK_COST_PARTITIONING_HEURISTIC_H

#include "landmark_heuristic.h"

namespace landmarks {
class LandmarkCostAssignment;

class LandmarkCostPartitioningHeuristic : public LandmarkHeuristic {
    std::unique_ptr<LandmarkCostAssignment> lm_cost_assignment;

    void check_unsupported_features(const plugins::Options &opts);
    void set_cost_assignment(const plugins::Options &opts);

    int get_heuristic_value(const State &state) override;
public:
    explicit LandmarkCostPartitioningHeuristic(const plugins::Options &opts);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
