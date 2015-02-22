#ifndef ABSTRACT_TASK_H
#define ABSTRACT_TASK_H

#include <string>
#include <utility>
#include <vector>

class GlobalOperator;

class AbstractTask {
public:
    AbstractTask() {}
    virtual ~AbstractTask() {}
    virtual int get_num_variables() const = 0;
    virtual int get_variable_domain_size(int var) const = 0;

    virtual int get_operator_cost(int index, bool is_axiom) const = 0;
    virtual const std::string &get_operator_name(int index, bool is_axiom) const = 0;
    virtual int get_num_operators() const = 0;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const = 0;
    virtual std::pair<int, int> get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const = 0;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const = 0;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const = 0;
    virtual std::pair<int, int> get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const = 0;
    virtual std::pair<int, int> get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const = 0;
    virtual const GlobalOperator *get_global_operator(int index, bool is_axiom) const = 0;

    virtual int get_num_axioms() const = 0;

    virtual int get_num_goals() const = 0;
    virtual std::pair<int, int> get_goal_fact(int index) const = 0;

    virtual std::vector<int> get_state_values(const std::vector<int> &global_state_values) const = 0;
};

#endif
