#ifndef COST_SATURATION_PROJECTION_H
#define COST_SATURATION_PROJECTION_H

#include "abstraction.h"

#include "../abstract_task.h"

#include "../pdbs/types.h"

#include <functional>
#include <vector>

class OperatorProxy;
class TaskProxy;
class VariablesProxy;

namespace pdbs {
class MatchTree;
}

namespace cost_saturation {
/* Precompute and store information about a task that is useful for projections. */
class TaskInfo {
    int num_variables;
    int num_operators;
    std::vector<FactPair> goals;

    /* Set bit at position op_id * num_variables + var to true iff the operator
       has a precondition or an effect on variable var. */
    std::vector<bool> mentioned_variables;

    /* Set bit at position op_id * num_variables + var to true iff the operator
       has a precondition and (different) effect on variable var. */
    std::vector<bool> pre_eff_variables;

    /* Set bit at position op_id * num_variables + var to true iff the operator
       has an effect on variable var. */
    std::vector<bool> effect_variables;

    int get_index(int op_id, int var) const {
        return op_id * num_variables + var;
    }
public:
    explicit TaskInfo(const TaskProxy &task_proxy);

    const std::vector<FactPair> &get_goals() const;
    int get_num_operators() const;
    bool operator_mentions_variable(int op_id, int var) const;
    bool operator_induces_self_loop(const pdbs::Pattern &pattern, int op_id) const;
    bool operator_is_active(const pdbs::Pattern &pattern, int op_id) const;
};

struct AbstractForwardOperator {
    int precondition_hash;
    int hash_effect;

    AbstractForwardOperator(
        int precondition_hash,
        int hash_effect)
        : precondition_hash(precondition_hash),
          hash_effect(hash_effect) {
    }
};

struct AbstractBackwardOperator {
    int concrete_operator_id;
    int hash_effect;

    AbstractBackwardOperator(
        int concrete_operator_id,
        int hash_effect)
        : concrete_operator_id(concrete_operator_id),
          hash_effect(hash_effect) {
    }
};


class ProjectionFunction : public AbstractionFunction {
    pdbs::Pattern pattern;
    // Multipliers for each pattern variable for perfect hash function.
    std::vector<std::size_t> hash_multipliers;

public:
    ProjectionFunction(
        const pdbs::Pattern &pattern, const std::vector<std::size_t> &hash_multipliers)
        : pattern(pattern),
          hash_multipliers(move(hash_multipliers)) {
        assert(pattern.size() == hash_multipliers.size());
    }

    virtual int get_abstract_state_id(const State &concrete_state) const override;
};


class Projection : public Abstraction {
    using Facts = std::vector<FactPair>;
    using OperatorCallback =
        std::function<void (Facts &, Facts &, Facts &, int, const std::vector<size_t> &, int)>;

    std::shared_ptr<TaskInfo> task_info;
    pdbs::Pattern pattern;

    std::vector<bool> looping_operators;

    std::vector<AbstractForwardOperator> abstract_forward_operators;

    std::vector<AbstractBackwardOperator> abstract_backward_operators;
    std::unique_ptr<pdbs::MatchTree> match_tree_backward;

    // Number of abstract states in the projection.
    int num_states;

    // Multipliers for each variable for perfect hash function.
    std::vector<std::size_t> hash_multipliers;

    // Domain size of each variable in the pattern.
    std::vector<int> pattern_domain_sizes;

    std::vector<int> goal_states;

    std::vector<int> compute_goal_states(
        const std::vector<int> &variable_to_pattern_index) const;

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
    void for_each_transition_impl(const Callback &callback) const {
        // Reuse vector to save allocations.
        std::vector<FactPair> abstract_facts;

        int num_operators = abstract_forward_operators.size();
        for (int op_id = 0; op_id < num_operators; ++op_id) {
            const AbstractForwardOperator &op = abstract_forward_operators[op_id];
            int concrete_op_id = abstract_backward_operators[op_id].concrete_operator_id;
            abstract_facts.clear();
            for (size_t i = 0; i < pattern.size(); ++i) {
                int var = pattern[i];
                if (!task_info->operator_mentions_variable(concrete_op_id, var)) {
                    abstract_facts.emplace_back(i, 0);
                }
            }

            bool has_next_match = true;
            while (has_next_match) {
                int state = op.precondition_hash;
                for (const FactPair &fact : abstract_facts) {
                    state += hash_multipliers[fact.var] * fact.value;
                }
                callback(Transition(state,
                                    concrete_op_id,
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
        const OperatorProxy &op,
        int cost,
        const std::vector<int> &variable_to_pattern_index,
        const VariablesProxy &variables,
        const OperatorCallback &callback) const;

    /*
      Return true iff all abstract facts hold in the given state.
    */
    bool is_consistent(
        std::size_t state_index,
        const std::vector<FactPair> &abstract_facts) const;

public:
    Projection(
        const TaskProxy &task_proxy,
        const std::shared_ptr<TaskInfo> &task_info,
        const pdbs::Pattern &pattern);
    virtual ~Projection() override;

    virtual std::vector<int> compute_goal_distances(
        const std::vector<int> &costs) const override;
    virtual std::vector<int> compute_saturated_costs(
        const std::vector<int> &h_values) const override;
    virtual int get_num_states() const override;
    virtual bool operator_is_active(int op_id) const override;
    virtual bool operator_induces_self_loop(int op_id) const override;
    virtual void for_each_transition(const TransitionCallback &callback) const override;
    virtual const std::vector<int> &get_goal_states() const override;

    virtual void dump() const override;
};
}

#endif
