#ifndef TASKS_PROJECTED_TASK_H
#define TASKS_PROJECTED_TASK_H

#include "delegating_task.h"

namespace extra_tasks {
/*
  Task transformation for performing a projection.

  We recommend using the factory function in
  projected_task_factory.h for creating ProjectedTasks.
*/
class ProjectedTask : public tasks::DelegatingTask {
    // Variable IDs and operator indices of the parent task.
    std::vector<int> variables;
    std::vector<int> operator_indices;

    // For each operator index, store the operator's preconditions and effects.
    std::vector<std::vector<FactPair>> operator_preconditions;
    std::vector<std::vector<FactPair>> operator_effects;

    std::vector<FactPair> goals;

    int convert_to_parent_variable(int var) const;
    FactPair convert_to_parent_fact(const FactPair &fact) const;
public:
    ProjectedTask(
        const std::shared_ptr<AbstractTask> &parent,
        std::vector<int> &&variables,
        std::vector<int> &&operator_indices,
        std::vector<std::vector<FactPair>> &&operator_preconditions,
        std::vector<std::vector<FactPair>> &&operator_effects,
        std::vector<FactPair> &&goals);
    virtual ~ProjectedTask() override = default;

    virtual int get_num_variables() const override;
    virtual std::string get_variable_name(int var) const override;
    virtual int get_variable_domain_size(int var) const override;
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;
    virtual std::string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual std::string get_operator_name(int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const override;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual int convert_operator_index_to_parent(int index) const override;

    virtual int get_num_axioms() const override;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;

    // TODO: should this rather override convert_state_values_from_parent?
    void convert_parent_state_values(std::vector<int> &values) const;
};
}

#endif
