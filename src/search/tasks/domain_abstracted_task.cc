#include "domain_abstracted_task.h"

#include "../global_state.h"

#include "../utils/system.h"

using namespace std;

namespace extra_tasks {
DomainAbstractedTask::DomainAbstractedTask(
    const shared_ptr<AbstractTask> &parent,
    vector<int> &&domain_size,
    vector<int> &&initial_state_values,
    vector<Fact> &&goals,
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

const string &DomainAbstractedTask::get_fact_name(const Fact &fact) const {
    return fact_names[fact.var][fact.value];
}

bool DomainAbstractedTask::are_facts_mutex(const Fact &, const Fact &) const {
    ABORT("DomainAbstractedTask doesn't support querying mutexes.");
}

Fact DomainAbstractedTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    return get_abstract_fact(
        parent->get_operator_precondition(op_index, fact_index, is_axiom));
}

Fact DomainAbstractedTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    return get_abstract_fact(
        parent->get_operator_effect_condition(
            op_index, eff_index, cond_index, is_axiom));
}

Fact DomainAbstractedTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_abstract_fact(
        parent->get_operator_effect(op_index, eff_index, is_axiom));
}

Fact DomainAbstractedTask::get_goal_fact(int index) const {
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
