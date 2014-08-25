#ifndef TASK_INTERFACE_H
#define TASK_INTERFACE_H

#include <cstddef>

class Operator;

class TaskInterface {
public:
    virtual std::size_t get_num_variables() const = 0;
    virtual std::size_t get_variable_domain_size(std::size_t var) const = 0;

    virtual int get_operator_cost(std::size_t index) const = 0;
    virtual std::size_t get_num_operators() const = 0;
    virtual std::size_t get_num_operator_preconditions(std::size_t index) const = 0;
    virtual std::pair<std::size_t, std::size_t> get_operator_precondition(
        std::size_t op_index, std::size_t fact_index) const = 0;
    virtual std::size_t get_num_operator_effects(std::size_t op_index) const = 0;
    virtual std::size_t get_num_operator_effect_conditions(
        std::size_t op_index, std::size_t eff_index) const = 0;
    virtual std::pair<std::size_t, std::size_t> get_operator_effect_condition(
        std::size_t op_index, std::size_t eff_index, std::size_t cond_index) const = 0;
    virtual std::pair<std::size_t, std::size_t> get_operator_effect(
        std::size_t op_index, std::size_t eff_index) const = 0;
    virtual const Operator *get_original_operator(std::size_t index) const = 0;

    virtual std::size_t get_num_axioms() const = 0;

    virtual std::size_t get_num_goals() const = 0;
    virtual std::pair<std::size_t, std::size_t> get_goal_fact(std::size_t index) const = 0;
};

#endif
