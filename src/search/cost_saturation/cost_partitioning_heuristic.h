#ifndef COST_SATURATION_COST_PARTITIONING_HEURISTIC_H
#define COST_SATURATION_COST_PARTITIONING_HEURISTIC_H

#include "types.h"

#include <vector>

namespace cost_saturation {
/*
  Compactly store cost-partitioned goal distances and use them to compute
  heuristic values for a given concrete state.

  For efficiency, we map a given concrete state to the corresponding abstract
  state IDs in each abstractions outside of this class. This allows us to
  compute the mapping only once instead of once for each abstraction order.

  We call the stored goal distances for an abstraction a lookup table.
  To save space, we only store lookup tables that contain positive estimates.
*/
class CostPartitioningHeuristic {
    // Allow this class to extract and compress information about unsolvable states.
    friend class UnsolvabilityHeuristic;

    struct LookupTable {
        int abstraction_id;
        /* h_values[i] is the goal distance of abstract state i under the cost
           function assigned to the associated abstraction. */
        std::vector<int> h_values;

        LookupTable(int abstraction_id, std::vector<int> &&h_values)
            : abstraction_id(abstraction_id),
              h_values(move(h_values)) {
        }
    };

    std::vector<LookupTable> lookup_tables;

public:
    void add_h_values(int abstraction_id, std::vector<int> &&h_values);

    /*
      Compute cost-partitioned heuristic value for a concrete state s. Callers
      need to precompute the abstract state IDs that s corresponds to in each
      abstraction with the get_abstract_state_ids() function. The result is the
      sum of all goal distances of the abstract states corresponding to s.
    */
    int compute_heuristic(const std::vector<int> &abstract_state_ids) const;

    // Return the number of useful abstractions.
    int get_num_lookup_tables() const;

    // Return the total number of stored heuristic values.
    int get_num_heuristic_values() const;

    // See class documentation.
    void mark_useful_abstractions(std::vector<bool> &useful_abstractions) const;
};
}

#endif
