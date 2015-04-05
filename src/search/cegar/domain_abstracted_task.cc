#include "domain_abstracted_task.h"

#include "../global_state.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace cegar {
DomainAbstractedTask::DomainAbstractedTask(
    shared_ptr<AbstractTask> parent,
    const VarToGroups &value_groups)
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

    for (const auto &pair : value_groups) {
        int var = pair.first;
        const ValueGroups &groups = pair.second;
        combine_values(var, groups);
    }
}

void DomainAbstractedTask::move_fact(int var, int before, int after) {
    cout << "Move fact " << var << ": " << before << " -> " << after << endl;
    assert(in_bounds(var, variable_domain));
    assert(in_bounds(before, task_index[var]));
    assert(in_bounds(after, task_index[var]));

    if (before == after)
        return;

    // Move each fact at most once.
    assert(task_index[var][before] == before);

    task_index[var][before] = after;
    // TODO: Use swap.
    fact_names[var][after] = fact_names[var][before];
    if (initial_state_data[var] == before)
        initial_state_data[var] = after;
    for (Fact &goal : goals) {
        if (var == goal.first && before == goal.second)
            goal.second = after;
    }
}

string DomainAbstractedTask::get_combined_fact_name(int var, const ValueGroup &values) const {
    stringstream name;
    string sep = "";
    for (int value : values) {
        name << sep << fact_names[var][value];
        sep = " OR ";
    }
    return name.str();
}

void DomainAbstractedTask::combine_values(int var, const ValueGroups &groups) {
    cout << "Combine " << var << ": ";
    for (const ValueGroup &group : groups)
        cout << group << " ";
    cout << endl;

    vector<string> combined_fact_names;
    unordered_set<int> group_union;
    int num_merged_values = 0;
    for (const ValueGroup &group : groups) {
        combined_fact_names.push_back(get_combined_fact_name(var, group));
        group_union.insert(group.begin(), group.end());
        num_merged_values += group.size();
    }
    assert(static_cast<int>(group_union.size()) == num_merged_values);

    int next_free_pos = 0;

    // Move all facts that are not part of groups to the front.
    for (int before = 0; before < variable_domain[var]; ++before) {
        if (group_union.count(before) == 0) {
            move_fact(var, before, next_free_pos++);
        }
    }
    int num_single_values = next_free_pos;
    assert(num_single_values + num_merged_values == variable_domain[var]);

    int new_domain_size= num_single_values + static_cast<int>(groups.size());
    fact_names[var].resize(new_domain_size);

    // Add new facts for merged groups.
    for (size_t group_id = 0; group_id < groups.size(); ++group_id) {
        const ValueGroup &group = groups[group_id];
        for (int before : group) {
            move_fact(var, before, next_free_pos);
        }
        fact_names[var][next_free_pos] = combined_fact_names[group_id];
        ++next_free_pos;
    }
    assert(next_free_pos = new_domain_size);

    // Update domain size.
    variable_domain[var] = new_domain_size;
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
