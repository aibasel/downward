#ifndef COST_SATURATION_UNSOLVABILITY_HEURISTIC_H
#define COST_SATURATION_UNSOLVABILITY_HEURISTIC_H

#include "types.h"

namespace cost_saturation {
/*
  Compactly store information about unsolvable abstract states.

  This class is a time and memory optimization. It extracts information about
  abstract states detected as unsolvable by at least one cost-partitioned
  heuristic and stores it in one bitvector per abstraction (if the abstraction
  recognizes at least one unsolvable state). Having extracted which states are
  unsolvable, we can delete all lookup tables from the cost-partitioned
  heuristics that only contain entries equal to 0 or infinity. This reduces the
  memory footprint and we need to access fewer lookup tables when evaluating a
  state.
*/
class UnsolvabilityHeuristic {
    struct UnsolvabilityInfo {
        int abstraction_id;
        std::vector<bool> unsolvable_states;

        UnsolvabilityInfo(int abstraction_id, std::vector<bool> &&unsolvable_states)
            : abstraction_id(abstraction_id),
              unsolvable_states(move(unsolvable_states)) {
        }
    };

    std::vector<UnsolvabilityInfo> unsolvability_infos;

public:
    UnsolvabilityHeuristic(const Abstractions &abstractions, CPHeuristics &cp_heuristics);

    bool is_unsolvable(const std::vector<int> &abstract_state_ids) const;
    void mark_useful_abstractions(std::vector<bool> &useful_abstractions) const;
};
}

#endif
