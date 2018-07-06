#ifndef COST_SATURATION_ORDER_GENERATOR_DYNAMIC_GREEDY_H
#define COST_SATURATION_ORDER_GENERATOR_DYNAMIC_GREEDY_H

#include "order_generator.h"
#include "scoring_functions.h"

namespace options {
class Options;
}

namespace cost_saturation {
class OrderGeneratorDynamicGreedy : public OrderGenerator {
    const ScoringFunction scoring_function;

    Order compute_dynamic_greedy_order_for_sample(
        const Abstractions &abstractions,
        const std::vector<int> &abstract_state_ids,
        std::vector<int> remaining_costs) const;

public:
    explicit OrderGeneratorDynamicGreedy(const options::Options &opts);

    virtual void initialize(
        const Abstractions &abstractions,
        const std::vector<int> &costs) override;

    virtual Order compute_order_for_state(
        const Abstractions &abstractions,
        const std::vector<int> &costs,
        const std::vector<int> &abstract_state_ids,
        bool verbose) override;
};
}

#endif
