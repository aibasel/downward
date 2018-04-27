#include "cost_partitioned_heuristic.h"

#include "abstraction.h"

#include "../utils/collections.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
LookupTable::LookupTable(
    int heuristic_index, vector<int> &&h_values)
    : heuristic_index(heuristic_index),
      h_values(move(h_values)) {
}


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

int CostPartitionedHeuristic::compute_heuristic(
    const Abstractions &abstractions, const State &state) const {
    int sum_h = 0;
    for (const LookupTable &lookup_table : lookup_tables) {
        const Abstraction &abstraction = *abstractions[lookup_table.heuristic_index];
        int state_id = abstraction.get_abstract_state_id(state);
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

const vector<LookupTable> &CostPartitionedHeuristic::get_lookup_tables() const {
    return lookup_tables;
}
}
