#ifndef COST_SATURATION_PROJECTION_H
#define COST_SATURATION_PROJECTION_H

#include "abstraction.h"

#include "../task_proxy.h"

#include "../algorithms/priority_queues.h"
#include "../heuristics/array_pool.h"
#include "../pdbs/pattern_database.h"
#include "../pdbs/types.h"

#include <vector>

namespace pdbs {
class AbstractOperator;
class MatchTree;
}

namespace cost_saturation {
struct AbstractForwardOperator {
    int concrete_operator_id;
    int precondition_hash;
    array_pool::ArrayPoolIndex unaffected_variables;
    int num_unaffected_variables;
    int hash_effect;

    AbstractForwardOperator(
        int concrete_operator_id,
        int precondition_hash,
        array_pool::ArrayPoolIndex unaffected_variables,
        int num_unaffected_variables,
        int hash_effect)
        : concrete_operator_id(concrete_operator_id),
          precondition_hash(precondition_hash),
          unaffected_variables(unaffected_variables),
          num_unaffected_variables(num_unaffected_variables),
          hash_effect(hash_effect) {
    }
};

/*
  TODO: Reduce code duplication with pdbs::PatternDatabase.
*/
class Projection : public Abstraction {
    using Facts = std::vector<FactPair>;
    using OperatorCallback =
        std::function<void (Facts &, Facts &, Facts &, int, const std::vector<size_t> &, int)>;

    TaskProxy task_proxy;
    pdbs::Pattern pattern;

    std::unique_ptr<array_pool::ArrayPool> unaffected_variables_per_operator;
    std::vector<AbstractForwardOperator> abstract_forward_operators;

    std::vector<pdbs::AbstractOperator> abstract_backward_operators;
    std::unique_ptr<pdbs::MatchTree> match_tree_backward;

    // Number of abstract states in the projection.
    int num_states;

    // Multipliers for each variable for perfect hash function.
    std::vector<std::size_t> hash_multipliers;

    /*
      For each variable store its index in the pattern or -1 if it is not in
      the pattern.
    */
    std::vector<int> variable_to_pattern_index;

    // Domain size of each variable in the pattern.
    std::vector<int> pattern_domain_sizes;

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
      Given an abstract state (represented as a vector of facts), compute the
      "next" fact. Return true iff there is a next fact.
    */
    bool increment_to_next_state(std::vector<FactPair> &facts) const;

    /*
      Apply a function to all transitions in the projection (including
      irrelevant transitions).
    */
    template<class Callback>
    void for_each_transition(const Callback &callback) const {
        // Reuse vector to save allocations.
        std::vector<FactPair> abstract_facts;

        int num_operators = abstract_forward_operators.size();
        for (int op_id = 0; op_id < num_operators; ++op_id) {
            const AbstractForwardOperator &op = abstract_forward_operators[op_id];
            abstract_facts.clear();
            for (int var : unaffected_variables_per_operator->get_slice(
                     op.unaffected_variables,
                     op.num_unaffected_variables)) {
                abstract_facts.emplace_back(var, 0);
            }
            bool has_next_match = true;
            while (has_next_match) {
                int state = op.precondition_hash;
                for (const FactPair &fact : abstract_facts) {
                    state += hash_multipliers[fact.var] * fact.value;
                }
                callback(Transition(state,
                                    op.concrete_operator_id,
                                    state + op.hash_effect));
                has_next_match = increment_to_next_state(abstract_facts);
            }
        }
    }

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
        const OperatorCallback &callback) const;

    /*
      Compute all abstract operators for a given concrete operator. Initialize
      data structures for initial call to recursive method multiply_out.
      variable_to_index maps variables in the task to their index in the
      pattern or -1.
    */
    void build_abstract_operators(
        const OperatorProxy &op, int cost,
        const std::vector<int> &variable_to_pattern_index,
        const VariablesProxy &variables,
        const OperatorCallback &callback) const;

    /*
      Return true iff all abstract facts hold in the given state.
    */
    bool is_consistent(
        std::size_t state_index,
        const std::vector<FactPair> &abstract_facts) const;

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
    virtual int get_num_states() const override;
    virtual const std::vector<int> &get_active_operators() const override;
    virtual const std::vector<int> &get_looping_operators() const override;
    virtual const std::vector<int> &get_goal_states() const override;

    virtual void dump() const override;
};
}

#endif
