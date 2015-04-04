#include "domain_abstracted_task.h"

#include "../global_state.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace cegar {
DomainAbstractedTask::DomainAbstractedTask(
    shared_ptr<AbstractTask> parent,
    const VariableToValues &fact_groups)
    : DelegatingTask(parent),
      initial_state_data(parent->get_initial_state_values()) {

    int num_vars = parent->get_num_variables();
    variable_domain.resize(num_vars);
    task_index.resize(num_vars);
    fact_names.resize(num_vars);
    for (int var = 0; var < num_vars; ++var) {
        int num_values = parent->get_variable_domain_size(var);
        variable_domain[var] = num_values;
        task_index[var].resize(num_values);
        fact_names[var].resize(num_values);
        for (int value = 0; value < num_values; ++value) {
            task_index[var][value] = value;
            fact_names[var][value] = parent->get_fact_name(var, value);
        }
    }

    for (const auto &group : fact_groups) {
        if (group.second.size() >= 2)
            combine_facts(group.first, group.second);
    }
}

void DomainAbstractedTask::move_fact(int var, int before, int after) {
    cout << "Move fact " << var << ": " << before << " -> " << after << endl;
    if (before == after)
        return;
    // Move each fact at most once.
    assert(task_index[var][before] == before);
    task_index[var][before] = after;
    fact_names[var][after] = fact_names[var][before];
    if (initial_state_data[var] == before)
        initial_state_data[var] = after;
    for (Fact &goal : goals) {
        if (var == goal.first && before == goal.second)
            goal.second = after;
    }
}

string DomainAbstractedTask::get_combined_fact_name(int var, const unordered_set<int> &values) const {
    stringstream name;
    string sep = "";
    for (int value : values) {
        name << sep << fact_names[var][value];
        sep = " OR ";
    }
    return name.str();
}

void DomainAbstractedTask::combine_facts(int var, const unordered_set<int> &values) {
    assert(values.size() >= 2);
    cout << "Combine " << var << ": " << values << endl;
    string combined_fact_name = get_combined_fact_name(var, values);

    int next_free_pos = 0;
    // Add all values that we want to keep.
    for (int before = 0; before < variable_domain[var]; ++before) {
        if (values.count(before) == 0) {
            move_fact(var, before, next_free_pos++);
        }
    }
    // Project all selected values onto single value.
    for (int before : values) {
        move_fact(var, before, next_free_pos);
    }
    assert(next_free_pos + static_cast<int>(values.size()) == variable_domain[var]);
    int num_values = next_free_pos + 1;
    variable_domain[var] = num_values;

    // Remove names of deleted facts.
    fact_names[var].erase(fact_names[var].begin() + num_values, fact_names[var].end());
    assert(static_cast<int>(fact_names[var].size()) == variable_domain[var]);

    // Set combined fact name.
    fact_names[var][next_free_pos] = combined_fact_name;
}

int DomainAbstractedTask::get_variable_domain_size(int var) const {
    return variable_domain[var];
}

const string &DomainAbstractedTask::get_fact_name(int var, int value) const {
    return fact_names[var][value];
}

pair<int, int> DomainAbstractedTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    return get_task_fact(parent->get_operator_precondition(op_index, fact_index, is_axiom));
}

pair<int, int> DomainAbstractedTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_task_fact(parent->get_operator_effect(op_index, eff_index, is_axiom));
}

pair<int, int> DomainAbstractedTask::get_goal_fact(int index) const {
    return get_task_fact(parent->get_goal_fact(index));
}

std::vector<int> DomainAbstractedTask::get_initial_state_values() const {
    return initial_state_data;
}

vector<int> DomainAbstractedTask::get_state_values(const GlobalState &global_state) const {
    int num_vars = variable_domain.size();
    vector<int> state_data(num_vars);
    for (int var = 0; var < num_vars; ++var) {
        int value = task_index[var][global_state[var]];
        state_data[var] = value;
    }
    return state_data;
}
}
