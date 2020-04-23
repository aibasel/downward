#ifndef CEGAR_COST_SATURATION_H
#define CEGAR_COST_SATURATION_H

#include "refinement_hierarchy.h"
#include "split_selector.h"

#include <memory>
#include <vector>

namespace utils {
class CountdownTimer;
class Duration;
class RandomNumberGenerator;
}

namespace cegar {
class CartesianHeuristicFunction;
class SubtaskGenerator;

/*
  Get subtasks from SubtaskGenerators, reduce their costs by wrapping
  them in ModifiedOperatorCostsTasks, compute Abstractions, move
  RefinementHierarchies from Abstractions to
  CartesianHeuristicFunctions, allow extracting
  CartesianHeuristicFunctions into AdditiveCartesianHeuristic.
*/
class CostSaturation {
    const std::vector<std::shared_ptr<SubtaskGenerator>> subtask_generators;
    const int max_states;
    const int max_non_looping_transitions;
    const double max_time;
    const bool use_general_costs;
    const PickSplit pick_split;
    utils::RandomNumberGenerator &rng;
    const bool debug;

    std::vector<CartesianHeuristicFunction> heuristic_functions;
    std::vector<int> remaining_costs;
    int num_abstractions;
    int num_states;
    int num_non_looping_transitions;

    void reset(const TaskProxy &task_proxy);
    void reduce_remaining_costs(const std::vector<int> &saturated_costs);
    std::shared_ptr<AbstractTask> get_remaining_costs_task(
        std::shared_ptr<AbstractTask> &parent) const;
    bool state_is_dead_end(const State &state) const;
    void build_abstractions(
        const std::vector<std::shared_ptr<AbstractTask>> &subtasks,
        const utils::CountdownTimer &timer,
        std::function<bool()> should_abort);
    void print_statistics(utils::Duration init_time) const;

public:
    CostSaturation(
        const std::vector<std::shared_ptr<SubtaskGenerator>> &subtask_generators,
        int max_states,
        int max_non_looping_transitions,
        double max_time,
        bool use_general_costs,
        PickSplit pick_split,
        utils::RandomNumberGenerator &rng,
        bool debug);

    std::vector<CartesianHeuristicFunction> generate_heuristic_functions(
        const std::shared_ptr<AbstractTask> &task);
};
}

#endif
