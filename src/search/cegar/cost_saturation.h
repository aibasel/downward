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
    std::shared_ptr<AbstractTask> task;
    std::vector<std::shared_ptr<SubtaskGenerator>> subtask_generators;
    const int max_states;
    utils::CountdownTimer timer;
    bool use_general_costs;
    PickSplit pick_split;
    std::vector<int> remaining_costs;
    std::vector<CartesianHeuristicFunction> heuristic_functions;
    int num_abstractions;
    int num_states;
    State initial_state;

    void reduce_remaining_costs(const std::vector<int> &saturated_costs);
    std::shared_ptr<AbstractTask> get_remaining_costs_task(
        std::shared_ptr<AbstractTask> &parent) const;
    bool initial_state_is_dead_end() const;
    bool may_build_another_abstraction();
    void build_abstractions(
        const std::vector<std::shared_ptr<AbstractTask>> &subtasks);
    void print_statistics() const;

public:
    explicit CostSaturation(const options::Options &opts);
    ~CostSaturation() = default;

    std::vector<CartesianHeuristicFunction> extract_heuristic_functions();
};
}

#endif
