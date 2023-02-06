#ifndef LANDMARKS_LANDMARK_COUNT_HEURISTIC_H
#define LANDMARKS_LANDMARK_COUNT_HEURISTIC_H

#include "landmark_heuristic.h"

namespace landmarks {
class LandmarkCountHeuristic : public LandmarkHeuristic {
    const bool dead_ends_reliable;

    std::vector<int> min_first_achiever_costs;
    std::vector<int> min_possible_achiever_costs;

    int get_min_cost_of_achievers(const std::set<int> &achievers);
    void compute_landmark_costs();

    int compute_heuristic_value(const State &state) override;
public:
    explicit LandmarkCountHeuristic(const plugins::Options &opts);

    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
