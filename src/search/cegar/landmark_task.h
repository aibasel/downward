#ifndef CEGAR_LANDMARK_TASK_H
#define CEGAR_LANDMARK_TASK_H

#include "utils.h"

#include "../delegating_task.h"
#include "../utilities.h"

#include <cassert>
#include <set>
#include <string>
#include <vector>

namespace cegar {
class DomainAbstractedTask : public DelegatingTask {
private:
    std::vector<int> initial_state_data;
    std::vector<Fact> goals;
    std::vector<int> variable_domain;
    std::vector<std::vector<std::string> > fact_names;
    std::vector<std::vector<int> > task_index;

    void move_fact(int var, int before, int after);
    void combine_facts(int var, const std::unordered_set<int> &values);

    int get_task_value(int var, int value) const {
        assert(in_bounds(var, task_index));
        assert(in_bounds(value, task_index[var]));
        return task_index[var][value];
    }
    std::pair<int, int> get_task_fact(Fact fact) const {
        return std::make_pair(fact.first, get_task_value(fact.first, fact.second));
    }

    std::string get_combined_fact_name(int var, const std::unordered_set<int> &values) const;
public:
    DomainAbstractedTask(
        std::shared_ptr<AbstractTask> parent,
        const VariableToValues &fact_groups);

    virtual int get_variable_domain_size(int var) const override;
    virtual const std::string &get_fact_name(int var, int value) const override;

    virtual std::pair<int, int> get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;

    virtual std::pair<int, int> get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
    virtual std::vector<int> get_state_values(const GlobalState &global_state) const override;
};
}

#endif
