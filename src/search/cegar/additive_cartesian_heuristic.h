#ifndef CEGAR_ADDITIVE_CARTESIAN_HEURISTIC_H
#define CEGAR_ADDITIVE_CARTESIAN_HEURISTIC_H

#include "split_selector.h"

#include "../heuristic.h"

#include <memory>
#include <vector>

/*
  Overview of classes relevant to Cartesian abstractions:

  AdditiveCartesianHeuristic
    Get subtasks from SubtaskGenerators, adjust their costs by wrapping
    them in ModifiedOperatorCostsTasks, compute Abstractions, move
    RefinementHierarchies from Abstractions to CartesianHeuristics,
    store CartesianHeuristics and compute overall heuristic by adding
    heuristic values of all CartesianHeuristics.

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

namespace utils {
class CountdownTimer;
}

namespace cegar {
class CartesianHeuristic;
class SubtaskGenerator;

/*
  TODO: All members except "heuristics" are needed only for creating
  the heuristic. Think about splitting heuristic creation and usage as
  in iPDB.
*/
class AdditiveCartesianHeuristic : public Heuristic {
    std::vector<std::shared_ptr<SubtaskGenerator>> subtask_generators;
    const int max_states;
    std::unique_ptr<utils::CountdownTimer> timer;
    bool use_general_costs;
    PickSplit pick_split;
    std::vector<int> remaining_costs;
    // TODO: Store split trees or thin wrappers directly.
    std::vector<std::unique_ptr<CartesianHeuristic>> heuristics;
    int num_abstractions;
    int num_states;

    void reduce_remaining_costs(const std::vector<int> &needed_costs);
    std::shared_ptr<AbstractTask> get_remaining_costs_task(
        std::shared_ptr<AbstractTask> &parent) const;
    bool may_build_another_abstraction();
    void build_abstractions(
        const std::vector<std::shared_ptr<AbstractTask>> &subtasks);
    void print_statistics() const;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit AdditiveCartesianHeuristic(const options::Options &options);
    ~AdditiveCartesianHeuristic() = default;
};
}

#endif
