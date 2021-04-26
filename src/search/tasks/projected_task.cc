#include "projected_task.h"

#include "../utils/collections.h"
#include "../utils/system.h"

#include <cassert>

using namespace std;

namespace extra_tasks {
ProjectedTask::ProjectedTask(
    const shared_ptr<AbstractTask> &parent,
    vector<int> &&variables,
    vector<int> &&operator_indices,
    vector<vector<FactPair>> &&operator_preconditions,
    vector<vector<FactPair>> &&operator_effects,
    vector<FactPair> &&goals)
    : DelegatingTask(parent),
      variables(move(variables)),
      operator_indices(move(operator_indices)),
      operator_preconditions(move(operator_preconditions)),
      operator_effects(move(operator_effects)),
      goals(move(goals)) {
    if (parent->get_num_axioms() > 0) {
        ABORT("ProjectedTask doesn't support axioms.");
    }
    if (has_conditional_effects(*parent)) {
        ABORT("ProjectedTask doesn't support conditional effects.");
    }
}

int ProjectedTask::convert_variable_to_parent(int var) const {
    assert(utils::in_bounds(var, variables));
    return variables[var];
}

FactPair ProjectedTask::convert_fact_to_parent(const FactPair &fact) const {
    return {
               convert_variable_to_parent(fact.var), fact.value
    };
}

int ProjectedTask::get_num_variables() const {
    return variables.size();
}

string ProjectedTask::get_variable_name(int var) const {
    int index = convert_variable_to_parent(var);
    return parent->get_variable_name(index);
}

int ProjectedTask::get_variable_domain_size(int var) const {
    int index = convert_variable_to_parent(var);
    return parent->get_variable_domain_size(index);
}

int ProjectedTask::get_variable_axiom_layer(int var) const {
    int index = convert_variable_to_parent(var);
    return parent->get_variable_axiom_layer(index);
}

int ProjectedTask::get_variable_default_axiom_value(int var) const {
    int index = convert_variable_to_parent(var);
    return parent->get_variable_default_axiom_value(index);
}

string ProjectedTask::get_fact_name(const FactPair &fact) const {
    return parent->get_fact_name(convert_fact_to_parent(fact));
}

bool ProjectedTask::are_facts_mutex(
    const FactPair &fact1, const FactPair &fact2) const {
    return parent->are_facts_mutex(
        convert_fact_to_parent(fact1),
        convert_fact_to_parent(fact2));
}

int ProjectedTask::get_operator_cost(int index, bool is_axiom) const {
    assert(!is_axiom);
    index = convert_operator_index_to_parent(index);
    return parent->get_operator_cost(index, is_axiom);
}

string ProjectedTask::get_operator_name(int index, bool is_axiom) const {
    assert(!is_axiom);
    index = convert_operator_index_to_parent(index);
    return parent->get_operator_name(index, is_axiom);
}

int ProjectedTask::get_num_operators() const {
    return operator_indices.size();
}

int ProjectedTask::get_num_operator_preconditions(
    int index, bool) const {
    return operator_preconditions[index].size();
}

FactPair ProjectedTask::get_operator_precondition(
    int op_index, int fact_index, bool) const {
    return operator_preconditions[op_index][fact_index];
}

int ProjectedTask::get_num_operator_effects(
    int op_index, bool) const {
    return operator_effects[op_index].size();
}

int ProjectedTask::get_num_operator_effect_conditions(int, int, bool) const {
    return 0;
}

FactPair ProjectedTask::get_operator_effect_condition(
    int, int, int, bool) const {
    ABORT("ProjectedTask doesn't support conditional effects.");
}

FactPair ProjectedTask::get_operator_effect(
    int op_index, int eff_index, bool) const {
    return operator_effects[op_index][eff_index];
}

int ProjectedTask::convert_operator_index_to_parent(int index) const {
    assert(utils::in_bounds(index, operator_indices));
    return operator_indices[index];
}

int ProjectedTask::get_num_axioms() const {
    assert(parent->get_num_axioms() == 0);
    return 0;
}

int ProjectedTask::get_num_goals() const {
    return goals.size();
}

FactPair ProjectedTask::get_goal_fact(int index) const {
    return goals[index];
}

vector<int> ProjectedTask::get_initial_state_values() const {
    vector<int> values = parent->get_initial_state_values();
    convert_state_values_from_parent(values);
    return values;
}

void ProjectedTask::convert_state_values_from_parent(
    vector<int> &values) const {
    vector<int> converted;
    converted.reserve(values.size());
    for (int var : variables) {
        converted.push_back(values[var]);
    }
    values.swap(converted);
}
}
