#ifndef COST_SATURATION_ORDER_OPTIMIZER_H
#define COST_SATURATION_ORDER_OPTIMIZER_H

#include "types.h"

namespace utils {
class CountdownTimer;
}

namespace cost_saturation {
/*
  Optimize the given order in-place via simple hill climbing.
*/
extern void optimize_order_with_hill_climbing(
    CPFunction cp_function,
    const utils::CountdownTimer &timer,
    const Abstractions &abstractions,
    const std::vector<int> &costs,
    const std::vector<int> &abstract_state_ids,
    Order &incumbent_order,
    CostPartitioningHeuristic &incumbent_cp,
    int incumbent_h_value,
    bool verbose);
}

#endif
