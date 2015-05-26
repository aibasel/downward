#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "../heuristic.h"
#include "../option_parser.h"

#include <memory>
#include <vector>

/*
  Overview of classes relevant to Cartesian abstractions:

  AdditiveCartesianHeuristic
    Get subtasks from Decompositions, adjust their costs by wrapping them in
    tasks::ModifiedCostsTasks, compute Abstractions, move SplitTrees from
    Abstractions to CartesianHeuristics, store CartesianHeuristics and
    compute overall heuristic by adding heuristic values of all
    CartesianHeuristics.

    Decomposition
      Create focused subtasks. NoDecomposition returns the original task.
      GoalDecomposition uses tasks::ModifiedGoalsTask to set a single goal
      fact. LandmarkDecomposition nests tasks::ModifiedGoalsTask and
      tasks::DomainAbstractedTask to focus on a single landmark fact.

    CartesianHeuristic
      Store SplitTree for looking up heuristic values efficiently.

  Abstraction
    Store the set of AbstractStates, use AbstractSearch to find abstract
    solutions, find flaws, use SplitSelector to select splits in case of
    ambiguities, break spurious solutions and maintain the refinement hierarchy
    in SplitTree.

    AbstractState
      Store and update abstract Domains and transitions.

      Domains
        Use a single boost::dynamic_bitset to store the Cartesian set of values
        in an abstract state.

    AbstractSearch
      Find an abstract solution using A*. Compute goal distances for abstract
      states.

    SplitSelector
      Strategies for selecting splits in case there are multiple possibilities.

    SplitTree
      Refinement hierarchy that has one tree node for each split. Contains
      helper nodes for splits that split off multiple facts.
*/

class CountdownTimer;
class Decomposition;
class GlobalState;

namespace cegar {
class CartesianHeuristic;

class AdditiveCartesianHeuristic : public Heuristic {
    const Options options;
    std::vector<std::shared_ptr<Decomposition> > decompositions;
    const int max_states;
    std::unique_ptr<CountdownTimer> timer;
    std::vector<int> remaining_costs;
    std::vector<std::shared_ptr<CartesianHeuristic> > heuristics;
    int num_abstractions;
    int num_states;

    void reduce_remaining_costs(const std::vector<int> &needed_costs);
    std::shared_ptr<AbstractTask> get_remaining_costs_task(
        std::shared_ptr<AbstractTask> parent) const;
    bool may_build_another_abstraction();
    void build_abstractions(const Decomposition &decomposition);
    void print_statistics() const;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit AdditiveCartesianHeuristic(const Options &options);
    ~AdditiveCartesianHeuristic() = default;
};
}

#endif
