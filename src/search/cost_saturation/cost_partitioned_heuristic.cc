#include "cost_partitioned_heuristic.h"

#include "../utils/collections.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
void CostPartitionedHeuristic::add_lookup_table_if_nonzero(
    int heuristic_id, vector<int> h_values, bool sparse) {
    if (!sparse ||
        any_of(h_values.begin(), h_values.end(), [](int h) {return h != 0; })) {
        lookup_tables.emplace_back(heuristic_id, move(h_values));
    }
}

int CostPartitionedHeuristic::compute_heuristic(
    const vector<int> &local_state_ids) const {
    int sum_h = 0;
    for (const LookupTable &lookup_table : lookup_tables) {
        assert(utils::in_bounds(lookup_table.heuristic_index, local_state_ids));
        int state_id = local_state_ids[lookup_table.heuristic_index];
        assert(utils::in_bounds(state_id, lookup_table.h_values));
        int h = lookup_table.h_values[state_id];
        assert(h >= 0);
        if (h == INF) {
            return INF;
        }
        sum_h += h;
        assert(sum_h >= 0);
    }
    return sum_h;
}

int CostPartitionedHeuristic::get_num_lookup_tables() const {
    return lookup_tables.size();
}

void CostPartitionedHeuristic::mark_useful_heuristics(
    vector<bool> useful_heuristics) const {
    for (const auto &lookup_table : lookup_tables) {
        assert(utils::in_bounds(lookup_table.heuristic_index, useful_heuristics));
        useful_heuristics[lookup_table.heuristic_index] = true;
    }
}
}
