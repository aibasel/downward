#ifndef DELEGATING_TASK_H
#define DELEGATING_TASK_H

#include "abstract_task.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>


/*
  Task transformation that delegates all calls to the corresponding methods of
  the parent task. You should inherit from this class instead of AbstractTask
  if you need specialized behavior for only some of the methods.
*/
class DelegatingTask : public AbstractTask {
protected:
    const std::shared_ptr<AbstractTask> parent;
public:
    explicit DelegatingTask(const std::shared_ptr<AbstractTask> parent);
    virtual ~DelegatingTask() override = default;

    virtual int get_num_variables() const override;
    virtual const std::string &get_variable_name(int var) const override;
    virtual int get_variable_domain_size(int var) const override;
    virtual const std::string &get_fact_name(int var, int value) const override;

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

    virtual std::vector<int> get_initial_state_values() const override;
    virtual std::vector<int> get_state_values(const GlobalState &global_state) const override;
};

#endif
