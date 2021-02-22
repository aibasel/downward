#ifndef TASKS_PROJECTED_TASK_H
#define TASKS_PROJECTED_TASK_H

#include "delegating_task.h"

namespace extra_tasks {
class ProjectedTask : public tasks::DelegatingTask {
    const std::vector<int> &pattern;
    std::vector<int> operator_indices;
    std::vector<std::vector<FactPair>> operator_preconditions;
    std::vector<std::vector<FactPair>> operator_effects;
    std::vector<FactPair> goals;

    /*
      Convert variable index of the abstracted task to
      the index of the same variable in the original task.
     */
    int get_original_variable_index(int index_in_pattern) const;
    //int get_abstracted_operator_index(int index_in_original) const;
    /*
      Convenience functions for changing the context of a
      given fact between the original and abstracted tasks.
     */
    FactPair convert_from_pattern_fact(const FactPair &fact) const;
public:
    ProjectedTask(
        const std::shared_ptr<AbstractTask> &parent,
        std::vector<int> &&pattern,
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

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;

    void convert_parent_state_values(std::vector<int> &values) const;
};
}

#endif
