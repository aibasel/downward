#ifndef COST_SATURATION_ORDER_GENERATOR_GREEDY_H
#define COST_SATURATION_ORDER_GENERATOR_GREEDY_H

#include "order_generator.h"
#include "greedy_order_utils.h"

namespace options {
class Options;
}

namespace cost_saturation {
class OrderGeneratorGreedy : public OrderGenerator {
    const ScoringFunction scoring_function;

    // Goal distances under the original cost function by abstraction.
    std::vector<std::vector<int>> h_values_by_abstraction;
    std::vector<int> stolen_costs_by_abstraction;

    double rate_abstraction(
        const std::vector<int> &abstract_state_ids, int abs_id) const;
    Order compute_static_greedy_order_for_sample(
        const std::vector<int> &abstract_state_ids, bool verbose) const;

public:
    explicit OrderGeneratorGreedy(const options::Options &opts);

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
