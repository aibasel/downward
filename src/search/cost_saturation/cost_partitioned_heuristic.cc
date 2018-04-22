#include "cost_partitioned_heuristic.h"

#include "abstraction.h"

#include "../utils/collections.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
CostPartitionedHeuristicValues::CostPartitionedHeuristicValues(
    int heuristic_index, vector<int> &&h_values)
    : heuristic_index(heuristic_index),
      h_values(move(h_values)) {
}


CostPartitionedHeuristic::CostPartitionedHeuristic() {
}

void CostPartitionedHeuristic::add_cp_heuristic_values(
    int heuristic_id, vector<int> h_values, bool sparse) {
    if (!sparse ||
        any_of(h_values.begin(), h_values.end(), [](int h) {return h != 0; })) {
        h_values_by_heuristic.emplace_back(heuristic_id, move(h_values));
    }
}

int CostPartitionedHeuristic::compute_heuristic(const vector<int> &local_state_ids) const {
    int sum_h = 0;
    for (const CostPartitionedHeuristicValues &cp_h_values : h_values_by_heuristic) {
        assert(utils::in_bounds(cp_h_values.heuristic_index, local_state_ids));
        int state_id = local_state_ids[cp_h_values.heuristic_index];
        assert(utils::in_bounds(state_id, cp_h_values.h_values));
        int h = cp_h_values.h_values[state_id];
        assert(h >= 0);
        if (h == INF) {
            return INF;
        }
        sum_h += h;
        assert(sum_h >= 0);
    }
    return sum_h;
}

int CostPartitionedHeuristic::compute_heuristic(
    const Abstractions &abstractions, const State &state) const {
    int sum_h = 0;
    for (const CostPartitionedHeuristicValues &cp_h_values : h_values_by_heuristic) {
        const Abstraction &abstraction = *abstractions[cp_h_values.heuristic_index];
        int state_id = abstraction.get_abstract_state_id(state);
        assert(utils::in_bounds(state_id, cp_h_values.h_values));
        int h = cp_h_values.h_values[state_id];
        assert(h >= 0);
        if (h == INF) {
            return INF;
        }
        sum_h += h;
        assert(sum_h >= 0);
    }
    return sum_h;
}

int CostPartitionedHeuristic::size() const {
    return h_values_by_heuristic.size();
}

const vector<CostPartitionedHeuristicValues>
&CostPartitionedHeuristic::get_h_values_by_heuristic() const {
    return h_values_by_heuristic;
}
}
