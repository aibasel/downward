#include "unsolvability_heuristic.h"

#include "abstraction.h"
#include "cost_partitioning_heuristic.h"

#include <algorithm>

using namespace std;

namespace cost_saturation {
UnsolvabilityHeuristic::UnsolvabilityHeuristic(
    const Abstractions &abstractions, CPHeuristics &cp_heuristics) {
    int num_abstractions = abstractions.size();

    // Create temporary data structure for storing unsolvable states.
    vector<vector<bool>> unsolvable;
    unsolvable.reserve(num_abstractions);
    for (int i = 0; i < num_abstractions; ++i) {
        unsolvable.emplace_back(abstractions[i]->get_num_states(), false);
    }

    // Collect unsolvable states from cost-partitioned heuristics.
    vector<bool> has_unsolvable_states(num_abstractions, false);
    for (auto &cp : cp_heuristics) {
        for (const auto &lookup_table : cp.lookup_tables) {
            for (size_t state = 0; state < lookup_table.h_values.size(); ++state) {
                if (lookup_table.h_values[state] == INF) {
                    unsolvable[lookup_table.abstraction_id][state] = true;
                    has_unsolvable_states[lookup_table.abstraction_id] = true;
                }
            }
        }
    }

    // Store unsolvable states.
    for (int i = 0; i < num_abstractions; ++i) {
        if (has_unsolvable_states[i]) {
            unsolvability_infos.emplace_back(i, move(unsolvable[i]));
        }
    }

    // Remove lookup tables that only contain entries equal to 0 or infinity.
    for (auto &cp : cp_heuristics) {
        auto &tables = cp.lookup_tables;
        tables.erase(
            remove_if(tables.begin(), tables.end(),
                      [](const CostPartitioningHeuristic::LookupTable &table) {
                          return all_of(table.h_values.begin(), table.h_values.end(),
                                        [](int h) {return h == 0 || h == INF;});
                      }), tables.end());
        tables.shrink_to_fit();
    }
}

bool UnsolvabilityHeuristic::is_unsolvable(const vector<int> &abstract_state_ids) const {
    for (const auto &info : unsolvability_infos) {
        if (info.unsolvable_states[abstract_state_ids[info.abstraction_id]]) {
            return true;
        }
    }
    return false;
}

void UnsolvabilityHeuristic::mark_useful_abstractions(
    vector<bool> &useful_abstractions) const {
    for (const auto &info : unsolvability_infos) {
        useful_abstractions[info.abstraction_id] = true;
    }
}
}
