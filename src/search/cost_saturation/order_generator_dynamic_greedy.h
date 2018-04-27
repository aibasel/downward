#ifndef COST_SATURATION_ORDER_GENERATOR_DYNAMIC_GREEDY_H
#define COST_SATURATION_ORDER_GENERATOR_DYNAMIC_GREEDY_H

#include "order_generator.h"
#include "scoring_functions.h"

namespace cost_saturation {
class OrderGeneratorDynamicGreedy : public OrderGenerator {
    const ScoringFunction scoring_function;
    int num_returned_orders;

    Order compute_dynamic_greedy_order_for_sample(
        const Abstractions &abstractions,
        const std::vector<int> &local_state_ids,
        std::vector<int> remaining_costs) const;

public:
    explicit OrderGeneratorDynamicGreedy(const options::Options &opts);

    virtual void initialize(
        const TaskProxy &task_proxy,
        const Abstractions &abstractions,
        const std::vector<int> &costs) override;

    virtual Order get_next_order(
        const TaskProxy &task_proxy,
        const Abstractions &abstractions,
        const std::vector<int> &costs,
        const std::vector<int> &local_state_ids,
        bool verbose) override;
};
}

#endif
