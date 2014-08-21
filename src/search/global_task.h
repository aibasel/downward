#ifndef GLOBAL_TASK_H
#define GLOBAL_TASK_H

#include "globals.h"
#include "operator.h"
#include "operator_cost.h"
#include "task_interface.h"

#include <cassert>
#include <cstddef>
#include <utility>


class GlobalTaskInterface : public TaskInterface {
public:
    std::size_t get_num_variables() const {return g_variable_domain.size(); }
    std::size_t get_variable_domain_size(std::size_t var) const {
        return g_variable_domain[var];
    }

    int get_operator_cost(std::size_t index) const {
        assert(index < g_operators.size());
        return g_operators[index].get_cost();
    }
    int get_adjusted_operator_cost(std::size_t index, OperatorCost cost_type) const {
        assert(index < g_operators.size());
        return get_adjusted_action_cost(g_operators[index], cost_type);
    }
    std::size_t get_num_operators() const {return g_operators.size(); }
    std::size_t get_num_operator_preconditions(std::size_t index) const;
    std::pair<std::size_t, std::size_t> get_operator_precondition(
        std::size_t op_index, std::size_t fact_index) const;
    std::size_t get_num_operator_effects(std::size_t op_index) const {
        return g_operators[op_index].get_pre_post().size();
    }
    std::size_t get_num_operator_effect_conditions(
        std::size_t op_index, std::size_t eff_index) const {
        return g_operators[op_index].get_pre_post()[eff_index].cond.size();
    }
    std::pair<std::size_t, std::size_t> get_operator_effect_condition(
        std::size_t op_index, std::size_t eff_index, std::size_t cond_index) const;
    std::pair<std::size_t, std::size_t> get_operator_effect(
        std::size_t op_index, std::size_t eff_index) const;
    const Operator *get_original_operator(std::size_t index) const;

    std::size_t get_num_axioms() const {return g_axioms.size(); }

    std::size_t get_num_goals() const {return g_goal.size(); }
    std::pair<std::size_t, std::size_t> get_goal_fact(std::size_t index) const {
        assert(index < get_num_goals());
        return g_goal[index];
    }
};

#endif
