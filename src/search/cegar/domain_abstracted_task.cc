#include "domain_abstracted_task.h"

#include "../global_state.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace cegar {
DomainAbstractedTask::DomainAbstractedTask(const std::shared_ptr<AbstractTask> parent,
    const VarToGroups &value_groups)
    : DelegatingTask(parent),
      initial_state_values(parent->get_initial_state_values()) {
    int num_vars = parent->get_num_variables();
    domain_size.resize(num_vars);
    value_map.resize(num_vars);
    fact_names.resize(num_vars);
    for (int var = 0; var < num_vars; ++var) {
        int num_values = parent->get_variable_domain_size(var);
        domain_size[var] = num_values;
        value_map[var].resize(num_values);
        fact_names[var].resize(num_values);
        for (int value = 0; value < num_values; ++value) {
            value_map[var][value] = value;
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
    assert(in_bounds(var, domain_size));
    assert(0 <= before && before < domain_size[var]);
    assert(0 <= after && after < domain_size[var]);

    if (before == after)
        return;

    // Move each fact at most once.
    assert(value_map[var][before] == before);

    value_map[var][before] = after;
    fact_names[var][after] = move(fact_names[var][before]);
    if (initial_state_values[var] == before)
        initial_state_values[var] = after;
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
    for (int before = 0; before < domain_size[var]; ++before) {
        if (group_union.count(before) == 0) {
            move_fact(var, before, next_free_pos++);
        }
    }
    int num_single_values = next_free_pos;
    assert(num_single_values + num_merged_values == domain_size[var]);

    // Add new facts for merged groups.
    for (size_t group_id = 0; group_id < groups.size(); ++group_id) {
        const ValueGroup &group = groups[group_id];
        for (int before : group) {
            move_fact(var, before, next_free_pos);
        }
        assert(in_bounds(next_free_pos, fact_names[var]));
        fact_names[var][next_free_pos] = move(combined_fact_names[group_id]);
        ++next_free_pos;
    }
    int new_domain_size = num_single_values + static_cast<int>(groups.size());
    assert(next_free_pos = new_domain_size);

    // Update domain size.
    fact_names[var].resize(new_domain_size);
    domain_size[var] = new_domain_size;
}

int DomainAbstractedTask::get_variable_domain_size(int var) const {
    return domain_size[var];
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
    return initial_state_values;
}

vector<int> DomainAbstractedTask::get_state_values(const GlobalState &global_state) const {
    int num_vars = domain_size.size();
    vector<int> state_data(num_vars);
    for (int var = 0; var < num_vars; ++var) {
        int value = value_map[var][global_state[var]];
        state_data[var] = value;
    }
    return state_data;
}
}
