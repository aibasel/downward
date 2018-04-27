#ifndef COST_SATURATION_COST_PARTITIONED_HEURISTIC_H
#define COST_SATURATION_COST_PARTITIONED_HEURISTIC_H

#include "types.h"

#include <vector>

namespace cost_saturation {
class CostPartitionedHeuristic {
    struct LookupTable {
        const int heuristic_index;
        const std::vector<int> h_values;

        LookupTable(int heuristic_index, std::vector<int> &&h_values)
          : heuristic_index(heuristic_index),
            h_values(move(h_values)) {
        }
    };

    std::vector<LookupTable> lookup_tables;

public:
    void add_lookup_table_if_nonzero(int heuristic_id, std::vector<int> h_values);
    int compute_heuristic(const std::vector<int> &local_state_ids) const;
    int get_num_lookup_tables() const;
    void mark_useful_heuristics(std::vector<bool> &useful_heuristics) const;
};
}

#endif
