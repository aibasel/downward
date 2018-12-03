#ifndef COST_SATURATION_COST_PARTITIONING_HEURISTIC_H
#define COST_SATURATION_COST_PARTITIONING_HEURISTIC_H

#include "types.h"

#include <vector>

namespace cost_saturation {
class CostPartitioningHeuristic {
    struct LookupTable {
        int heuristic_index;
        /* h_values[i] is the goal distance of abstract state i under the cost
           function assigned to the associated abstraction. */
        std::vector<int> h_values;

        LookupTable(int heuristic_index, std::vector<int> &&h_values)
            : heuristic_index(heuristic_index),
              h_values(move(h_values)) {
        }
    };

    std::vector<LookupTable> lookup_tables;

public:
    // Add the given lookup table only if it contains at least one value > 0.
    void add_lookup_table_if_nonzero(int heuristic_id, std::vector<int> h_values);

    // Sum up the values stored for all abstract states.
    int compute_heuristic(const std::vector<int> &abstract_state_ids) const;

    // Return the number of stored lookup tables.
    int get_num_lookup_tables() const;

    // Return the total number of stored heuristic values.
    int get_num_heuristic_values() const;

    // Add the indices of stored lookup tables to the bit vector.
    void mark_useful_heuristics(std::vector<bool> &useful_heuristics) const;
};
}

#endif
