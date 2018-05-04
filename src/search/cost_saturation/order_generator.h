#ifndef COST_SATURATION_ORDER_GENERATOR_H
#define COST_SATURATION_ORDER_GENERATOR_H

#include "types.h"

#include <vector>

namespace cost_saturation {
class OrderGenerator {
public:
    OrderGenerator() = default;
    virtual ~OrderGenerator() = default;

    virtual void initialize(
        const Abstractions &abstractions,
        const std::vector<int> &costs) = 0;

    virtual Order compute_order_for_state(
        const Abstractions &abstractions,
        const std::vector<int> &costs,
        const std::vector<int> &abstract_state_ids,
        bool verbose) = 0;
};
}

#endif
