#ifndef COST_SATURATION_ORDER_GENERATOR_RANDOM_H
#define COST_SATURATION_ORDER_GENERATOR_RANDOM_H

#include "order_generator.h"

namespace utils {
class RandomNumberGenerator;
}

namespace cost_saturation {
class OrderGeneratorRandom : public OrderGenerator {
    const std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::vector<int> random_order;

public:
    explicit OrderGeneratorRandom(const options::Options &opts);

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
