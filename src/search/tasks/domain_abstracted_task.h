#ifndef TASKS_DOMAIN_ABSTRACTED_TASK_H
#define TASKS_DOMAIN_ABSTRACTED_TASK_H

#include "../delegating_task.h"

#include "../utils/collections.h"

#include <cassert>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ExtraTasks {
using ValueGroup = std::vector<int>;
using ValueGroups = std::vector<ValueGroup>;
using VarToGroups = std::unordered_map<int, ValueGroups>;

/*
  Task transformation for performing domain abstraction.

  We recommend using the factory function in
  domain_abstracted_task_factory.h for creating DomainAbstractedTasks.
*/
class DomainAbstractedTask : public DelegatingTask {
    const std::vector<int> domain_size;
    const std::vector<int> initial_state_values;
    const std::vector<std::pair<int, int>> goals;
    const std::vector<std::vector<std::string>> fact_names;
    const std::vector<std::vector<int>> value_map;

    int get_abstract_value(int var, int value) const {
        assert(Utils::in_bounds(var, value_map));
        assert(Utils::in_bounds(value, value_map[var]));
        return value_map[var][value];
    }

    std::pair<int, int> get_abstract_fact(const std::pair<int, int> &fact) const {
        return std::make_pair(fact.first, get_abstract_value(fact.first, fact.second));
    }

public:
    DomainAbstractedTask(
        const std::shared_ptr<AbstractTask> parent,
        std::vector<int> &&domain_size,
        std::vector<int> &&initial_state_values,
        std::vector<std::pair<int, int>> &&goals,
        std::vector<std::vector<std::string>> &&fact_names,
        std::vector<std::vector<int>> &&value_map);

    virtual int get_variable_domain_size(int var) const override;
    virtual const std::string &get_fact_name(int var, int value) const override;
    virtual bool are_facts_mutex(
        const std::pair<int, int> &fact1, const std::pair<int, int> &fact2) const override;

    virtual std::pair<int, int> get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;

    virtual std::pair<int, int> get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
    virtual std::vector<int> get_state_values(
        const GlobalState &global_state) const override;
};
}

#endif
