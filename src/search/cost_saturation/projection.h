#ifndef COST_SATURATION_PROJECTION_H
#define COST_SATURATION_PROJECTION_H

#include "abstraction.h"

#include "../task_proxy.h"

#include "../algorithms/priority_queues.h"
#include "../pdbs/types.h"

#include <vector>

namespace pdbs {
class AbstractOperator;
class MatchTree;
}

namespace cost_saturation {
/*
  TODO: Reduce code duplication with pdbs::PatternDatabase.
*/
class Projection : public Abstraction {
    const TaskProxy task_proxy;
    const pdbs::Pattern pattern;

    std::vector<pdbs::AbstractOperator> abstract_operators;
    std::unique_ptr<pdbs::MatchTree> match_tree;

    // Number of abstract states in the projection.
    int num_states;

    // Multipliers for each variable for perfect hash function.
    std::vector<std::size_t> hash_multipliers;

    std::vector<int> goal_states;

    // Operators inducing state-changing transitions.
    std::vector<int> active_operators;

    // Operators inducing self-loops.
    std::vector<int> looping_operators;

    // Reuse the queue to save memory allocations.
    mutable priority_queues::AdaptiveQueue<size_t> pq;

    // Return true iff op has an effect on a variable in the pattern.
    bool is_operator_relevant(const OperatorProxy &op) const;

    /* Return true iff there is no variable in the pattern for which op
       has a precondition and (different) effect. */
    bool operator_induces_loop(const OperatorProxy &op) const;

    std::vector<int> compute_active_operators() const;
    std::vector<int> compute_looping_operators() const;
    std::vector<int> compute_goal_states() const;

    /*
      Recursive method; called by build_abstract_operators. In the case
      of a precondition with value = -1 in the concrete operator, all
      multiplied-out abstract operators are computed, i.e., for all
      possible values of the variable (with precondition = -1), one
      abstract operator with a concrete value (!= -1) is computed.
    */
    void multiply_out(
        int pos, int cost, int op_id,
        std::vector<FactPair> &prev_pairs,
        std::vector<FactPair> &pre_pairs,
        std::vector<FactPair> &eff_pairs,
        const std::vector<FactPair> &effects_without_pre,
        const VariablesProxy &variables,
        std::vector<pdbs::AbstractOperator> &abstract_operators) const;

    /*
      Compute all abstract operators for a given concrete operator. Initialize
      data structures for initial call to recursive method multiply_out.
      variable_to_index maps variables in the task to their index in the
      pattern or -1.
    */
    void build_abstract_operators(
        const OperatorProxy &op, int cost,
        const std::vector<int> &variable_to_index,
        const VariablesProxy &variables,
        std::vector<pdbs::AbstractOperator> &abstract_operators) const;

    /*
      Use Dijkstra's algorithm to compute all goal distances. If 'transitions'
      is given it is filled with all transitions visited during the execution
      of Dijkstra's algorithm, i.e., all transitions that may be part of any
      plan from any abstract state in the projection.
    */
    std::vector<int> compute_distances(
        const std::vector<int> &costs,
        std::vector<Transition> *transitions = nullptr) const;

    /*
      Return true iff the given state is an abstract goal state.
    */
    bool is_goal_state(
        const std::size_t state_index,
        const std::vector<FactPair> &abstract_goals,
        const VariablesProxy &variables) const;

    /*
      Use the given concrete state to calculate the index of the corresponding
      abstract state. This is only used for table lookup during search.
    */
    std::size_t hash_index(const State &state) const;

protected:
    virtual std::vector<int> compute_saturated_costs(
        const std::vector<int> &h_values,
        int num_operators) const override;

    virtual void release_transition_system_memory() override;

public:
    Projection(const TaskProxy &task_proxy, const pdbs::Pattern &pattern);
    virtual ~Projection() override;

    virtual int get_abstract_state_id(const State &concrete_state) const override;
    virtual std::vector<int> compute_goal_distances(
        const std::vector<int> &costs) const override;
    virtual std::vector<Transition> get_transitions() const override;
    virtual int get_num_states() const override;
    virtual const std::vector<int> &get_active_operators() const override;
    virtual const std::vector<int> &get_looping_operators() const override;
    virtual const std::vector<int> &get_goal_states() const override;

    virtual void dump() const override;
};
}

#endif
