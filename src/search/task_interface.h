#ifndef TASK_INTERFACE_H
#define TASK_INTERFACE_H

#include <string>
#include <utility>

class GlobalOperator;

class TaskInterface {
public:
    TaskInterface() {}
    virtual ~TaskInterface() {}
    virtual int get_num_variables() const = 0;
    virtual int get_variable_domain_size(int var) const = 0;

    virtual int get_operator_cost(int index, bool is_axiom) const = 0;
    virtual std::string get_operator_name(int index, bool is_axiom) const = 0;
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
};

#endif
