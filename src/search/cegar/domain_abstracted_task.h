#ifndef CEGAR_DOMAIN_ABSTRACTED_TASK_H
#define CEGAR_DOMAIN_ABSTRACTED_TASK_H

#include "../delegating_task.h"
#include "../utilities.h"

#include <cassert>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cegar {
using Fact = std::pair<int, int>;
using ValueGroup = std::vector<int>;
using ValueGroups = std::vector<ValueGroup>;
using VarToGroups = std::unordered_map<int, ValueGroups>;

class DomainAbstractedTask : public DelegatingTask {
private:
    const std::vector<int> domain_size;
    const std::vector<int> initial_state_values;
    const std::vector<Fact> goals;
    const std::vector<std::vector<std::string> > fact_names;
    const std::vector<std::vector<int> > value_map;

    int get_task_value(int var, int value) const {
        assert(in_bounds(var, value_map));
        assert(in_bounds(value, value_map[var]));
        return value_map[var][value];
    }

    Fact get_task_fact(Fact fact) const {
        return std::make_pair(fact.first, get_task_value(fact.first, fact.second));
    }

public:
    DomainAbstractedTask(
        const std::shared_ptr<AbstractTask> parent,
        std::vector<int> && domain_size,
        std::vector<int> && initial_state_values,
        std::vector<Fact> && goals,
        std::vector<std::vector<std::string> > && fact_names,
        std::vector<std::vector<int> > && value_map);

    virtual int get_variable_domain_size(int var) const override;
    virtual const std::string &get_fact_name(int var, int value) const override;

    virtual std::pair<int, int> get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;

    virtual std::pair<int, int> get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
    virtual std::vector<int> get_state_values(
        const GlobalState &global_state) const override;
};
}

#endif
