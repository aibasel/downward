#ifndef COST_SATURATION_DIVERSIFIER_H
#define COST_SATURATION_DIVERSIFIER_H

#include "types.h"

namespace cost_saturation {
class CostPartitionedHeuristic;

class Diversifier {
    std::vector<std::vector<int>> abstract_state_ids_by_sample;
    std::vector<int> portfolio_h_values;

public:
    explicit Diversifier(std::vector<std::vector<int>> &&abstract_state_ids_by_sample);

    bool is_diverse(const CostPartitionedHeuristic &cp);
};
}

#endif
