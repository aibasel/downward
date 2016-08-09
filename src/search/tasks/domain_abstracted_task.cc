#include "domain_abstracted_task.h"

#include "../global_state.h"

#include "../utils/system.h"

using namespace std;

namespace extra_tasks {
DomainAbstractedTask::DomainAbstractedTask(
    const shared_ptr<AbstractTask> &parent,
    vector<int> &&domain_size,
    vector<int> &&initial_state_values,
    vector<FactPair> &&goals,
    vector<vector<string>> &&fact_names,
    vector<vector<int>> &&value_map)
    : DelegatingTask(parent),
      domain_size(move(domain_size)),
      initial_state_values(move(initial_state_values)),
      goals(move(goals)),
      fact_names(move(fact_names)),
      value_map(move(value_map)) {
}

int DomainAbstractedTask::get_variable_domain_size(int var) const {
    return domain_size[var];
}

int DomainAbstractedTask::get_variable_axiom_layer(int) const {
    ABORT("DomainAbstractedTask doesn't support axioms.");
}

int DomainAbstractedTask::get_variable_default_axiom_value(int) const {
    ABORT("DomainAbstractedTask doesn't support axioms.");
}

const string &DomainAbstractedTask::get_fact_name(const FactPair &fact) const {
    return fact_names[fact.var][fact.value];
}

bool DomainAbstractedTask::are_facts_mutex(const FactPair &, const FactPair &) const {
    ABORT("DomainAbstractedTask doesn't support querying mutexes.");
}

FactPair DomainAbstractedTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    return get_abstract_fact(
        parent->get_operator_precondition(op_index, fact_index, is_axiom));
}

int DomainAbstractedTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    int num_conditions = parent->get_num_operator_effect_conditions(
        op_index, eff_index, is_axiom);
    if (num_conditions > 0) {
        ABORT("DomainAbstractedTask doesn't support conditional effects.");
    }
    return num_conditions;
}

FactPair DomainAbstractedTask::get_operator_effect_condition(
    int, int, int, bool) const {
    ABORT("DomainAbstractedTask doesn't support conditional effects.");
}

FactPair DomainAbstractedTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_abstract_fact(
        parent->get_operator_effect(op_index, eff_index, is_axiom));
}

int DomainAbstractedTask::get_num_axioms() const {
    int num_axioms = parent->get_num_axioms();
    if (num_axioms > 0) {
        ABORT("DomainAbstractedTask doesn't support axioms.");
    }
    return num_axioms;
}

FactPair DomainAbstractedTask::get_goal_fact(int index) const {
    return get_abstract_fact(parent->get_goal_fact(index));
}

vector<int> DomainAbstractedTask::get_initial_state_values() const {
    return initial_state_values;
}

void DomainAbstractedTask::convert_state_values_from_parent(
    vector<int> &values) const {
    int num_vars = domain_size.size();
    for (int var = 0; var < num_vars; ++var) {
        int old_value = values[var];
        int new_value = value_map[var][old_value];
        values[var] = new_value;
    }
}
}
