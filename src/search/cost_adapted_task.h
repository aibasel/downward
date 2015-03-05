#ifndef COST_ADAPTED_TASK_H
#define COST_ADAPTED_TASK_H

#include "abstract_task.h"
#include "operator_cost.h"

class OptionParser;
class Options;

class CostAdaptedTask : public AbstractTask {
    const OperatorCost cost_type;
    const AbstractTask &parent;
public:
    explicit CostAdaptedTask(const Options &opts);
    ~CostAdaptedTask();

    virtual int get_num_variables() const override;
    virtual int get_variable_domain_size(int var) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
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

    virtual std::vector<int> get_state_values(const GlobalState &global_state) const override;
};

void add_cost_type_option_to_parser(OptionParser &parser);

#endif
