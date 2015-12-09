#include "domain_abstracted_task.h"

#include "../global_state.h"

using namespace std;

namespace tasks {
DomainAbstractedTask::DomainAbstractedTask(
    const shared_ptr<AbstractTask> parent,
    vector<int> &&domain_size,
    vector<int> &&initial_state_values,
    vector<Fact> &&goals,
    vector<vector<string>> &&fact_names,
    vector<vector<int>> &&value_map)
    : DelegatingTask(parent),
      domain_size(domain_size),
      initial_state_values(initial_state_values),
      goals(goals),
      fact_names(fact_names),
      value_map(value_map) {
}

int DomainAbstractedTask::get_variable_domain_size(int var) const {
    return domain_size[var];
}

const string &DomainAbstractedTask::get_fact_name(int var, int value) const {
    return fact_names[var][value];
}

pair<int, int> DomainAbstractedTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    return get_task_fact(parent->get_operator_precondition(
                             op_index, fact_index, is_axiom));
}

pair<int, int> DomainAbstractedTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_task_fact(parent->get_operator_effect(
                             op_index, eff_index, is_axiom));
}

pair<int, int> DomainAbstractedTask::get_goal_fact(int index) const {
    return get_task_fact(parent->get_goal_fact(index));
}

vector<int> DomainAbstractedTask::get_initial_state_values() const {
    return initial_state_values;
}

vector<int> DomainAbstractedTask::get_state_values(
    const GlobalState &global_state) const {
    int num_vars = domain_size.size();
    vector<int> state_data(num_vars);
    for (int var = 0; var < num_vars; ++var) {
        int value = value_map[var][global_state[var]];
        state_data[var] = value;
    }
    return state_data;
}
}
