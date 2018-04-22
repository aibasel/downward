#ifndef COST_SATURATION_COST_PARTITIONED_HEURISTIC_H
#define COST_SATURATION_COST_PARTITIONED_HEURISTIC_H

#include "types.h"

#include <vector>

class State;

namespace cost_saturation {
struct CostPartitionedHeuristicValues {
    const int heuristic_index;
    const std::vector<int> h_values;

public:
    CostPartitionedHeuristicValues(int heuristic_index, std::vector<int> &&h_values);
};


class CostPartitionedHeuristic {
    std::vector<CostPartitionedHeuristicValues> h_values_by_heuristic;

public:
    CostPartitionedHeuristic();

    void add_cp_heuristic_values(
        int heuristic_id, std::vector<int> h_values, bool sparse);

    // Use the first overload for precomputed local state IDs.
    int compute_heuristic(const std::vector<int> &local_state_ids) const;
    int compute_heuristic(const Abstractions &abstractions, const State &state) const;

    int size() const;

    const std::vector<CostPartitionedHeuristicValues> &get_h_values_by_heuristic() const;
};
}

#endif
