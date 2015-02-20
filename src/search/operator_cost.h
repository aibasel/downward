#ifndef OPERATOR_COST_H
#define OPERATOR_COST_H

#include "task_interface.h"

class GlobalOperator;
class Options;
class OptionParser;

enum OperatorCost {NORMAL = 0, ONE = 1, PLUSONE = 2, MAX_OPERATOR_COST};

class AdaptCosts : public TaskInterface {
    const TaskInterface &base;
    const OperatorCost cost_type;
public:
    explicit AdaptCosts(const TaskInterface &base_, const Options &opts);
    ~AdaptCosts();

    virtual int get_num_variables() const override;
    virtual int get_variable_domain_size(int var) const override;

    virtual int get_operator_cost(int, bool) const override;
    virtual const std::string &get_operator_name(int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual const GlobalOperator *get_global_operator(int index, bool is_axiom) const override;

    virtual int get_num_axioms() const override;

    virtual int get_num_goals() const override;
    virtual std::pair<int, int> get_goal_fact(int index) const override;
};

int get_adjusted_action_cost(int cost, OperatorCost cost_type);
int get_adjusted_action_cost(const GlobalOperator &op, OperatorCost cost_type);
void add_cost_type_option_to_parser(OptionParser &parser);

#endif
