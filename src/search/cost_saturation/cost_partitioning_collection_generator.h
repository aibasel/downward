#ifndef COST_SATURATION_COST_PARTITIONING_COLLECTION_GENERATOR_H
#define COST_SATURATION_COST_PARTITIONING_COLLECTION_GENERATOR_H

#include "types.h"

#include <memory>
#include <vector>

class RandomWalkSampler;
class State;
class SuccessorGenerator;
class TaskProxy;

namespace utils {
class RandomNumberGenerator;
}

namespace cost_saturation {
class Abstraction;
class CostPartitionedHeuristic;
class OrderGenerator;

class CostPartitioningCollectionGenerator {
    const std::shared_ptr<OrderGenerator> cp_generator;
    const int max_orders;
    const double max_time;
    const bool diversify;
    const int num_samples;
    const double max_optimization_time;
    const bool steepest_ascent;
    const std::shared_ptr<utils::RandomNumberGenerator> rng;

public:
    CostPartitioningCollectionGenerator(
        const std::shared_ptr<OrderGenerator> &cp_generator,
        int max_orders,
        double max_time,
        bool diversify,
        int num_samples,
        double max_optimization_time,
        bool steepest_ascent,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng);
    ~CostPartitioningCollectionGenerator();

    std::vector<CostPartitionedHeuristic> get_cost_partitionings(
        const TaskProxy &task_proxy,
        const Abstractions &abstractions,
        const std::vector<int> &costs,
        CPFunction cp_function) const;
};
}

#endif
