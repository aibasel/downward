#ifndef COST_SATURATION_COST_PARTITIONING_HEURISTIC_H
#define COST_SATURATION_COST_PARTITIONING_HEURISTIC_H

#include "types.h"

#include <vector>

namespace cost_saturation {
/*
  Compactly store cost-partitioned goal distances and compute heuristic values
  by summing the goal distances of abstract states corresponding to a given
  concrete state.

  For efficiency, users of this class need to store the abstractions and map a
  given concrete state to the corresponding abstract state IDs in all
  abstractions themselves. This allows them to compute the mapping only once
  instead of for each order.

  We call an abstraction A useful if h^A(s) > 0 for at least one state s. To
  save space, we only store h values for useful abstractions.
*/
class CostPartitioningHeuristic {
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
    void add_h_values(int abstraction_id, std::vector<int> h_values);

    /*
      Compute cost-partitioned heuristic value for a concrete state s. Callers
      need to precompute the abstract state IDs that s corresponds to in all
      abstractions (not only useful abstractions). The result is the sum of all
      stored heuristic values for abstract states corresponding to s.
    */
    int compute_heuristic(const std::vector<int> &abstract_state_ids) const;

    // Return the number of useful abstractions.
    int get_num_lookup_tables() const;

    // Return the total number of stored heuristic values.
    int get_num_heuristic_values() const;

    // An abstraction A is useful if h^A(s) > 0 for at least one state s (see above).
    void mark_useful_abstractions(std::vector<bool> &useful_abstractions) const;
};
}

#endif
