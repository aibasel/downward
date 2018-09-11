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
    const std::vector<FactPair> goals;
    const std::vector<std::vector<std::string>> fact_names;
    const std::vector<std::vector<int>> value_map;

    int get_abstract_value(const FactPair &fact) const {
        assert(utils::in_bounds(fact.var, value_map));
        assert(utils::in_bounds(fact.value, value_map[fact.var]));
        return value_map[fact.var][fact.value];
    }

    FactPair get_abstract_fact(const FactPair &fact) const {
        return FactPair(fact.var, get_abstract_value(fact));
    }

public:
    DomainAbstractedTask(
        const std::shared_ptr<AbstractTask> &parent,
        std::vector<int> &&domain_size,
        std::vector<int> &&initial_state_values,
        std::vector<FactPair> &&goals,
        std::vector<std::vector<std::string>> &&fact_names,
        std::vector<std::vector<int>> &&value_map);

    virtual int get_variable_domain_size(int var) const override;
    virtual std::string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;

    virtual FactPair get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
    virtual void convert_state_values_from_parent(
        std::vector<int> &values) const override;
};
}

#endif
