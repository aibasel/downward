#ifndef COST_SATURATION_ORDER_OPTIMIZER_H
#define COST_SATURATION_ORDER_OPTIMIZER_H

#include "types.h"

namespace utils {
class CountdownTimer;
}

namespace cost_saturation {
extern void do_hill_climbing(
    CPFunction cp_function,
    const utils::CountdownTimer &timer,
    const Abstractions &abstractions,
    const std::vector<int> &costs,
    const std::vector<int> &local_state_ids,
    Order &incumbent_order,
    CostPartitionedHeuristic &incumbent_cp,
    int incumbent_h_value,
    bool steepest_ascent,
    bool sparse,
    bool verbose);
}

#endif
