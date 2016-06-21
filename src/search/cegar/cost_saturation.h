#ifndef CEGAR_COST_SATURATION_H
#define CEGAR_COST_SATURATION_H

#include "split_selector.h"

#include "../utils/countdown_timer.h"

#include <memory>
#include <vector>

/*
  Overview of classes relevant to Cartesian abstractions:

  CostSaturation
    Get subtasks from SubtaskGenerators, adjust their costs by wrapping
    them in ModifiedOperatorCostsTasks, compute Abstractions, move
    RefinementHierarchies from Abstractions to CartesianHeuristicFunctions,
    allow extracting CartesianHeuristicFunctions into AdditiveCartesianHeuristic.

    AdditiveCartesianHeuristic
      Store CartesianHeuristicFunctions and compute overall heuristic
      by adding heuristic values of all CartesianHeuristicFunctions.

    SubtaskGenerator
      Create focused subtasks. TaskDuplicator returns copies of the
      original task. GoalDecomposition uses ModifiedGoalsTask to set a
      single goal fact. LandmarkDecomposition nests ModifiedGoalsTask
      and DomainAbstractedTask to focus on a single landmark fact.

    CartesianHeuristic
      Store RefinementHierarchy for looking up heuristic values
      efficiently.

  Abstraction
    Store the set of AbstractStates, use AbstractSearch to find
    abstract solutions, find flaws, use SplitSelector to select splits
    in case of ambiguities, break spurious solutions and maintain the
    RefinementHierarchy.

    AbstractState
      Store and update abstract Domains and transitions.

      Domains
        Store the Cartesian set of values in an abstract state.

    AbstractSearch
      Find an abstract solution using A*. Compute goal distances for
      abstract states.

    SplitSelector
      Strategies for selecting splits in case there are multiple
      possibilities.

    RefinementHierarchy
      Directed acyclic graph that has an inner tree node for each split
      and a leaf for all current abstract states in the abstraction.
      Contains helper nodes for splits that split off multiple facts.
*/

namespace cegar {
class CartesianHeuristicFunction;
class SubtaskGenerator;

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
