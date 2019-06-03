#ifndef COST_SATURATION_UNSOLVABILITY_HEURISTIC_H
#define COST_SATURATION_UNSOLVABILITY_HEURISTIC_H

#include "types.h"

namespace cost_saturation {
class UnsolvabilityHeuristic {
    struct UnsolvabilityInfo {
        int abstraction_id;
        std::vector<bool> unsolvable_states;

        UnsolvabilityInfo(int abstraction_id, std::vector<bool> &&unsolvable_states)
            : abstraction_id(abstraction_id),
              unsolvable_states(move(unsolvable_states)) {
        }
    };

    std::vector<UnsolvabilityInfo> unsolvable_states;

public:
    explicit UnsolvabilityHeuristic(const Abstractions &abstractions);

    bool is_unsolvable(const std::vector<int> &abstract_state_ids) const;
    void mark_useful_abstractions(std::vector<bool> &useful_abstractions) const;
};
}

#endif
