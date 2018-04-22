#ifndef COST_SATURATION_ORDER_GENERATOR_H
#define COST_SATURATION_ORDER_GENERATOR_H

#include "types.h"

#include <memory>
#include <vector>

class TaskProxy;

namespace options {
class Options;
class OptionParser;
}

namespace cost_saturation {
class Abstraction;
class CostPartitionedHeuristic;

class OrderGenerator {
public:
    OrderGenerator() = default;
    virtual ~OrderGenerator() = default;

    virtual void initialize(
        const TaskProxy &task_proxy,
        const std::vector<std::unique_ptr<Abstraction>> &abstractions,
        const std::vector<int> &costs) = 0;

    virtual Order get_next_order(
        const TaskProxy &task_proxy,
        const std::vector<std::unique_ptr<Abstraction>> &abstractions,
        const std::vector<int> &costs,
        const std::vector<int> &local_state_ids,
        bool verbose) = 0;

    virtual bool has_next_cost_partitioning() const {
        return true;
    }
};
}

#endif
