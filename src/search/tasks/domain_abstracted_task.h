#ifndef TASKS_DOMAIN_ABSTRACTED_TASK_H
#define TASKS_DOMAIN_ABSTRACTED_TASK_H

#include "delegating_task.h"

#include "../utils/collections.h"

#include <cassert>
#include <string>
#include <utility>
#include <vector>


namespace extra_tasks {
/*
  Task transformation for performing domain abstraction.

  We recommend using the factory function in
  domain_abstracted_task_factory.h for creating DomainAbstractedTasks.
*/
class DomainAbstractedTask : public tasks::DelegatingTask {
    const std::vector<int> domain_size;
    const std::vector<int> initial_state_values;
    const std::vector<Fact> goals;
    const std::vector<std::vector<std::string>> fact_names;
    const std::vector<std::vector<int>> value_map;

    int get_abstract_value(const Fact &fact) const {
        assert(utils::in_bounds(fact.var, value_map));
        assert(utils::in_bounds(fact.value, value_map[fact.var]));
        return value_map[fact.var][fact.value];
    }

    Fact get_abstract_fact(const Fact &fact) const {
        return Fact(fact.var, get_abstract_value(fact));
    }

public:
    DomainAbstractedTask(
        const std::shared_ptr<AbstractTask> &parent,
        std::vector<int> &&domain_size,
        std::vector<int> &&initial_state_values,
        std::vector<Fact> &&goals,
        std::vector<std::vector<std::string>> &&fact_names,
        std::vector<std::vector<int>> &&value_map);

    virtual int get_variable_domain_size(int var) const override;
    virtual const std::string &get_fact_name(const Fact &fact) const override;
    virtual bool are_facts_mutex(
        const Fact &fact1, const Fact &fact2) const override;

    virtual Fact get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual Fact get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual Fact get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;

    virtual Fact get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
    virtual std::vector<int> get_state_values(
        const GlobalState &global_state) const override;
};
}

#endif
