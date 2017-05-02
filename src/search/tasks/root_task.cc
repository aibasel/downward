#include "root_task.h"

#include "../global_operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state_registry.h"

#include "../utils/collections.h"

#include <algorithm>
#include <cassert>
#include <memory>
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

RootTask::RootTask(
    vector<ExplicitVariable> &&variables,
    vector<vector<set<FactPair>>> &&mutexes,
    vector<ExplicitOperator> &&operators,
    vector<ExplicitOperator> &&axioms,
    vector<int> &&initial_state_values,
    vector<FactPair> &&goals)
    : variables(move(variables)),
      mutexes(move(mutexes)),
      operators(move(operators)),
      axioms(move(axioms)),
      initial_state_values(move(initial_state_values)),
      goals(move(goals)),
      evaluated_axioms_on_initial_state(false) {
    assert(run_sanity_check());
}

const ExplicitVariable &RootTask::get_variable(int var) const {
    assert(utils::in_bounds(var, variables));
    return variables[var];
}

const ExplicitEffect &RootTask::get_effect(
    int op_id, int effect_id, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_id, is_axiom);
    assert(utils::in_bounds(effect_id, op.effects));
    return op.effects[effect_id];
}

const ExplicitOperator &RootTask::get_operator_or_axiom(
    int index, bool is_axiom) const {
    if (is_axiom) {
        assert(utils::in_bounds(index, axioms));
        return axioms[index];
    } else {
        assert(utils::in_bounds(index, operators));
        return operators[index];
    }
}

bool RootTask::run_sanity_check() const {
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

void RootTask::evaluate_axioms_on_initial_state() const {
    if (!axioms.empty()) {
        // HACK this should not have to go through a state registry.
        // HACK on top of the HACK above: this should not use globals.
        StateRegistry state_registry(
            *this, *g_state_packer, *g_axiom_evaluator, initial_state_values);
        initial_state_values = state_registry.get_initial_state().get_values();
    }
    evaluated_axioms_on_initial_state = true;
}

int RootTask::get_num_variables() const {
    return variables.size();
}

string RootTask::get_variable_name(int var) const {
    return get_variable(var).name;
}

int RootTask::get_variable_domain_size(int var) const {
    return get_variable(var).domain_size;
}

int RootTask::get_variable_axiom_layer(int var) const {
    return get_variable(var).axiom_layer;
}

int RootTask::get_variable_default_axiom_value(int var) const {
    return get_variable(var).axiom_default_value;
}

string RootTask::get_fact_name(const FactPair &fact) const {
    assert(utils::in_bounds(fact.value, get_variable(fact.var).fact_names));
    return get_variable(fact.var).fact_names[fact.value];
}

bool RootTask::are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const {
    if (fact1.var == fact2.var) {
        // Same variable: mutex iff different value.
        return fact1.value != fact2.value;
    }
    assert(utils::in_bounds(fact1.var, mutexes));
    assert(utils::in_bounds(fact1.value, mutexes[fact1.var]));
    return bool(mutexes[fact1.var][fact1.value].count(fact2));
}

int RootTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).cost;
}

string RootTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).name;
}

int RootTask::get_num_operators() const {
    return operators.size();
}

int RootTask::get_num_operator_preconditions(
    int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).preconditions.size();
}

FactPair RootTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_index, is_axiom);
    assert(utils::in_bounds(fact_index, op.preconditions));
    return op.preconditions[fact_index];
}

int RootTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).effects.size();
}

int RootTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).conditions.size();
}

FactPair RootTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const ExplicitEffect &effect = get_effect(op_index, eff_index, is_axiom);
    assert(utils::in_bounds(cond_index, effect.conditions));
    return effect.conditions[cond_index];
}

FactPair RootTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).fact;
}

OperatorID RootTask::get_global_operator_id(OperatorID id) const {
    return id;
}

int RootTask::get_num_axioms() const {
    return axioms.size();
}

int RootTask::get_num_goals() const {
    return goals.size();
}

FactPair RootTask::get_goal_fact(int index) const {
    assert(utils::in_bounds(index, goals));
    return goals[index];
}

vector<int> RootTask::get_initial_state_values() const {
    if (!evaluated_axioms_on_initial_state) {
        evaluate_axioms_on_initial_state();
    }
    return initial_state_values;
}

void RootTask::convert_state_values(
    vector<int> &,
    const AbstractTask *ancestor_task) const {
    if (ancestor_task != this) {
        ABORT("Invalid state conversion");
    }
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

ExplicitOperator explicit_operator_from_global_operator(const GlobalOperator &op, bool is_axiom) {
    vector<FactPair> preconditions;
    preconditions.reserve(op.get_preconditions().size());
    for (const GlobalCondition &condition : op.get_preconditions()) {
        preconditions.emplace_back(condition.var, condition.val);
    }

    vector<ExplicitEffect> effects;
    effects.reserve(op.get_effects().size());
    for (const GlobalEffect &eff : op.get_effects()) {
        vector<FactPair> eff_conditions;
        eff_conditions.reserve(eff.conditions.size());
        for (const GlobalCondition &condition : eff.conditions) {
            eff_conditions.emplace_back(condition.var, condition.val);
        }
        effects.emplace_back(eff.var, eff.val, move(eff_conditions));
    }

    return ExplicitOperator(
        move(preconditions),
        move(effects),
        op.get_cost(),
        op.get_name(),
        is_axiom);
}

shared_ptr<RootTask> create_root_task(
    const vector<string> &variable_name,
    const vector<int> &variable_domain,
    const vector<vector<string>> &fact_names,
    const vector<int> &axiom_layers,
    const vector<int> &default_axiom_values,
    const vector<vector<set<FactPair>>> &inconsistent_facts,
    const vector<int> &initial_state_data,
    const vector<pair<int, int>> &goal,
    const vector<GlobalOperator> &global_operators,
    const vector<GlobalOperator> &global_axioms) {

    vector<ExplicitVariable> variables;
    variables.reserve(variable_domain.size());
    for (int i = 0; i < static_cast<int>(variable_domain.size()); ++i) {
        string name = variable_name[i];
        vector<string> var_fact_names = fact_names[i];
        variables.emplace_back(
            variable_domain[i],
            move(name),
            move(var_fact_names),
            axiom_layers[i],
            default_axiom_values[i]);
    }

    vector<vector<set<FactPair>>> mutexes = inconsistent_facts;


    vector<ExplicitOperator> operators;
    operators.reserve(operators.size());
    for (const GlobalOperator &op : global_operators) {
        operators.push_back(explicit_operator_from_global_operator(op, false));
    }

    vector<ExplicitOperator> axioms;
    axioms.reserve(axioms.size());
    for (const GlobalOperator &axiom : global_axioms) {
        axioms.push_back(explicit_operator_from_global_operator(axiom, true));
    }

    vector<int> initial_state_values = initial_state_data;

    vector<FactPair> goals;
    goals.reserve(goal.size());
    for (pair<int, int> goal : goal) {
        goals.emplace_back(goal.first, goal.second);
    }

    return make_shared<RootTask>(
        move(variables),
        move(mutexes),
        move(operators),
        move(axioms),
        move(initial_state_values),
        move(goals));
}

static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return g_root_task();
}

static PluginShared<AbstractTask> _plugin("no_transform", _parse);
}
