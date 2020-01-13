#ifndef COST_SATURATION_ORDER_GENERATOR_H
#define COST_SATURATION_ORDER_GENERATOR_H

#include "types.h"

#include <vector>

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace cost_saturation {
class OrderGenerator {
protected:
    const std::shared_ptr<utils::RandomNumberGenerator> rng;
public:
    explicit OrderGenerator(const options::Options &opts);
    virtual ~OrderGenerator() = default;

    virtual void initialize(
        const Abstractions &abstractions, const std::vector<int> &costs) = 0;

    virtual Order compute_order_for_state(
        const std::vector<int> &abstract_state_ids, bool verbose) = 0;
};

extern void add_common_order_generator_options(options::OptionParser &parser);
}

#endif
