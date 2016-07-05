#ifndef CEGAR_COST_SATURATION_H
#define CEGAR_COST_SATURATION_H

#include "split_selector.h"

#include "../utils/countdown_timer.h"

#include <memory>
#include <vector>

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
    utils::CountdownTimer timer;
    const bool use_general_costs;
    const PickSplit pick_split;
    std::vector<int> remaining_costs;
    std::vector<CartesianHeuristicFunction> heuristic_functions;
    int num_abstractions;
    int num_states;

    void reset(const TaskProxy &task_proxy);
    void reduce_remaining_costs(const std::vector<int> &saturated_costs);
    std::shared_ptr<AbstractTask> get_remaining_costs_task(
        std::shared_ptr<AbstractTask> &parent) const;
    bool state_is_dead_end(const State &state) const;
    void build_abstractions(
        const std::vector<std::shared_ptr<AbstractTask>> &subtasks,
        std::function<bool()> may_continue);
    void print_statistics() const;

public:
    CostSaturation(
        std::vector<std::shared_ptr<SubtaskGenerator>> subtask_generators,
        int max_states,
        int max_time,
        bool use_general_costs,
        PickSplit pick_split);

    std::vector<CartesianHeuristicFunction> generate_heuristic_functions(
        const std::shared_ptr<AbstractTask> &task);
};
}

#endif
