#include "delegating_task.h"

using namespace std;


DelegatingTask::DelegatingTask(const shared_ptr<AbstractTask> parent)
    : parent(parent) {
}

int DelegatingTask::get_num_variables() const {
    return parent->get_num_variables();
}

const string &DelegatingTask::get_variable_name(int var) const {
    return parent->get_variable_name(var);
}

int DelegatingTask::get_variable_domain_size(int var) const {
    return parent->get_variable_domain_size(var);
}

const string &DelegatingTask::get_fact_name(int var, int value) const {
    return parent->get_fact_name(var, value);
}

int DelegatingTask::get_operator_cost(int index, bool is_axiom) const {
    return parent->get_operator_cost(index, is_axiom);
}

const string &DelegatingTask::get_operator_name(int index, bool is_axiom) const {
    return parent->get_operator_name(index, is_axiom);
}

int DelegatingTask::get_num_operators() const {
    return parent->get_num_operators();
}

int DelegatingTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return parent->get_num_operator_preconditions(index, is_axiom);
}

pair<int, int> DelegatingTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    return parent->get_operator_precondition(op_index, fact_index, is_axiom);
}

int DelegatingTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return parent->get_num_operator_effects(op_index, is_axiom);
}

int DelegatingTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return parent->get_num_operator_effect_conditions(op_index, eff_index, is_axiom);
}

pair<int, int> DelegatingTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return parent->get_operator_effect_condition(op_index, eff_index, cond_index, is_axiom);
}

pair<int, int> DelegatingTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return parent->get_operator_effect(op_index, eff_index, is_axiom);
}

const GlobalOperator *DelegatingTask::get_global_operator(int index, bool is_axiom) const {
    return parent->get_global_operator(index, is_axiom);
}

int DelegatingTask::get_num_axioms() const {
    return parent->get_num_axioms();
}

int DelegatingTask::get_num_goals() const {
    return parent->get_num_goals();
}

pair<int, int> DelegatingTask::get_goal_fact(int index) const {
    return parent->get_goal_fact(index);
}

std::vector<int> DelegatingTask::get_initial_state_values() const {
    return parent->get_initial_state_values();
}

vector<int> DelegatingTask::get_state_values(const GlobalState &global_state) const {
    return parent->get_state_values(global_state);
}
