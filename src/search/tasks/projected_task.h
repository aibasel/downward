#ifndef TASKS_PROJECTED_TASK_H
#define TASKS_PROJECTED_TASK_H

#include "delegating_task.h"
#include "../pdbs/types.h"
#include "../task_proxy.h"

namespace tasks {
class ProjectedTask : public DelegatingTask {
    const pdbs::Pattern &pattern;
    std::unordered_map<int, int> var_to_index; // where to find variable in pattern
    std::vector<int> operator_indices;
    std::vector<std::vector<FactPair>> operator_preconditions;
    std::vector<std::vector<FactPair>> operator_effects;
    std::vector<FactPair> goals;

    /*
      Convert variable index of the abstracted task to
      the index of the same variable in the original task.
     */
    int get_original_variable_index(int index_in_pattern) const;
    /*
      Convert index with which the variable is accessed in
      the original task to an index under which the variable
      resides in the pattern.
     */
    int get_pattern_variable_index(int index_in_original) const;
    //int get_abstracted_operator_index(int index_in_original) const;
    /*
      Convenience functions for changing the context of a
      given fact between the original and abstracted tasks.
     */
    FactPair convert_from_pattern_fact(const FactPair &fact) const;
    FactPair convert_from_original_fact(const FactPair &fact) const;
public:
    explicit ProjectedTask(
        const std::shared_ptr<AbstractTask> &parent,
        const pdbs::Pattern &pattern);

    int get_num_variables() const override;
    std::string get_variable_name(int var) const override;
    int get_variable_domain_size(int var) const override;
    int get_variable_axiom_layer(int var) const override;
    int get_variable_default_axiom_value(int var) const override;
    std::string get_fact_name(const FactPair &fact) const override;
    bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    int get_operator_cost(int index, bool is_axiom) const override;
    std::string get_operator_name(int index, bool is_axiom) const override;
    int get_num_operators() const override;
    int get_num_operator_preconditions(int index, bool is_axiom) const override;
    FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    int get_num_operator_effects(int op_index, bool is_axiom) const override;
    int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual int convert_operator_index_to_parent(int index) const override;

    int get_num_goals() const override;
    FactPair get_goal_fact(int index) const override;

    std::vector<int> get_initial_state_values() const override;

    void convert_parent_state_values(std::vector<int> &values) const;
};
}

#endif // TASKS_PDB_ABSTRACTED_TASK_H
