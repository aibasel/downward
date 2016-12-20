#include "explicit_task.h"

#include "../utils/system.h"

#include <algorithm>
#include <unordered_set>

using namespace std;

namespace tasks {
#ifndef NDEBUG
static bool check_fact(const FactPair &fact) {
    /*
      We don't put this check into the FactPair ctor to allow (-1, -1)
      facts. Here, we want to make sure that the fact is valid.
    */
    return fact.var >= 0 && fact.value >= 0;
}
#endif

ExplicitTask::ExplicitTask(
    const shared_ptr<AbstractTask> &parent,
    vector<ExplicitVariable> &&variables,
    vector<vector<set<FactPair>>> &&mutex_facts,
    vector<ExplicitOperator> &&operators,
    vector<ExplicitOperator> &&axioms,
    vector<int> &&initial_state_values,
    vector<FactPair> &&goals)
    : DelegatingTask(parent),
      variables(move(variables)),
      mutexes(move(mutex_facts)),
      operators(move(operators)),
      axioms(move(axioms)),
      initial_state_values(move(initial_state_values)),
      goals(move(goals)) {
    assert(run_sanity_check());
}

bool ExplicitTask::run_sanity_check() const {
    assert(initial_state_values.size() == variables.size());

    function<bool(const ExplicitOperator &op)> is_axiom =
        [](const ExplicitOperator &op) {
            return op.is_an_axiom;
        };
    assert(none_of(operators.begin(), operators.end(), is_axiom));
    assert(all_of(axioms.begin(), axioms.end(), is_axiom));

    // Check that each variable occurs at most once in the goal.
    unordered_set<int> goal_vars;
    for (const FactPair &goal: goals) {
        goal_vars.insert(goal.var);
    }
    assert(goal_vars.size() == goals.size());
    return true;
}

int ExplicitTask::get_num_variables() const {
    return variables.size();
}

string ExplicitTask::get_variable_name(int var) const {
    return get_variable(var).name;
}

int ExplicitTask::get_variable_domain_size(int var) const {
    return get_variable(var).domain_size;
}

int ExplicitTask::get_variable_axiom_layer(int var) const {
    return get_variable(var).axiom_layer;
}

int ExplicitTask::get_variable_default_axiom_value(int var) const {
    return get_variable(var).axiom_default_value;
}

string ExplicitTask::get_fact_name(const FactPair &fact) const {
    assert(utils::in_bounds(fact.value, get_variable(fact.var).fact_names));
    return get_variable(fact.var).fact_names[fact.value];
}

bool ExplicitTask::are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const {
    if (fact1.var == fact2.var) {
        // Same variable: mutex iff different value.
        return fact1.value != fact2.value;
    }
    assert(utils::in_bounds(fact1.var, mutexes));
    assert(utils::in_bounds(fact1.value, mutexes[fact1.var]));
    return bool(mutexes[fact1.var][fact1.value].count(fact2));
}

int ExplicitTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).cost;
}

string ExplicitTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).name;
}

int ExplicitTask::get_num_operators() const {
    return operators.size();
}

int ExplicitTask::get_num_operator_preconditions(
    int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).preconditions.size();
}

FactPair ExplicitTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_index, is_axiom);
    assert(utils::in_bounds(fact_index, op.preconditions));
    return op.preconditions[fact_index];
}

int ExplicitTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).effects.size();
}

int ExplicitTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).conditions.size();
}

FactPair ExplicitTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const ExplicitEffect &effect = get_effect(op_index, eff_index, is_axiom);
    assert(utils::in_bounds(cond_index, effect.conditions));
    return effect.conditions[cond_index];
}

FactPair ExplicitTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).fact;
}

int ExplicitTask::get_num_axioms() const {
    return axioms.size();
}

int ExplicitTask::get_num_goals() const {
    return goals.size();
}

FactPair ExplicitTask::get_goal_fact(int index) const {
    assert(utils::in_bounds(index, goals));
    return goals[index];
}

vector<int> ExplicitTask::get_initial_state_values() const {
    return initial_state_values;
}


ExplicitVariable::ExplicitVariable(
    int domain_size,
    string &&name,
    vector<string> &&fact_names,
    int axiom_layer,
    int axiom_default_value)
    : domain_size(domain_size),
      name(move(name)),
      fact_names(move(fact_names)),
      axiom_layer(axiom_layer),
      axiom_default_value(axiom_default_value) {
    assert(domain_size >= 1);
    assert(static_cast<int>(this->fact_names.size()) == domain_size);
    assert(axiom_layer >= -1);
    assert(axiom_default_value >= -1);
}


ExplicitEffect::ExplicitEffect(
    int var, int value, vector<FactPair> &&conditions)
    : fact(var, value), conditions(move(conditions)) {
    assert(check_fact(FactPair(var, value)));
    assert(all_of(
               this->conditions.begin(), this->conditions.end(), check_fact));
}


ExplicitOperator::ExplicitOperator(
    vector<FactPair> &&preconditions,
    vector<ExplicitEffect> &&effects,
    int cost,
    const string &name,
    bool is_an_axiom)
    : preconditions(move(preconditions)),
      effects(move(effects)),
      cost(cost),
      name(name),
      is_an_axiom(is_an_axiom) {
    assert(all_of(
               this->preconditions.begin(),
               this->preconditions.end(),
               check_fact));
    assert(cost >= 0);
}
}
