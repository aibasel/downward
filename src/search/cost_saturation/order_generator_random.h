#ifndef COST_SATURATION_ORDER_GENERATOR_RANDOM_H
#define COST_SATURATION_ORDER_GENERATOR_RANDOM_H

#include "order_generator.h"

namespace cost_saturation {
class OrderGeneratorRandom : public OrderGenerator {
    std::vector<int> random_order;
public:
    explicit OrderGeneratorRandom(const options::Options &opts);

    virtual void initialize(
        const Abstractions &abstractions,
        const std::vector<int> &costs) override;

    virtual Order compute_order_for_state(
        const std::vector<int> &abstract_state_ids,
        bool verbose) override;
};
}

#endif
