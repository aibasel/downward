#include "root_task.h"

#include "../global_operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state_registry.h"

#include "../utils/collections.h"

#include <cassert>

using namespace std;

namespace tasks {
static GlobalOperator &get_operator_or_axiom(int index, bool is_axiom) {
    if (is_axiom) {
        assert(utils::in_bounds(index, g_axioms));
        return g_axioms[index];
    } else {
        assert(utils::in_bounds(index, g_operators));
        return g_operators[index];
    }
}

int RootTask::get_num_variables() const {
    return g_variable_domain.size();
}

string RootTask::get_variable_name(int var) const {
    return g_variable_name[var];
}

int RootTask::get_variable_domain_size(int var) const {
    return g_variable_domain[var];
}

int RootTask::get_variable_axiom_layer(int var) const {
    return g_axiom_layers[var];
}

int RootTask::get_variable_default_axiom_value(int var) const {
    return g_default_axiom_values[var];
}

string RootTask::get_fact_name(const FactPair &fact) const {
    return g_fact_names[fact.var][fact.value];
}

bool RootTask::are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const {
    return are_mutex(fact1, fact2);
}

int RootTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_cost();
}

string RootTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_name();
}

int RootTask::get_num_operators() const {
    return g_operators.size();
}

int RootTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_preconditions().size();
}

FactPair RootTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const GlobalOperator &op = get_operator_or_axiom(op_index, is_axiom);
    const GlobalCondition &precondition = op.get_preconditions()[fact_index];
    return FactPair(precondition.var, precondition.val);
}

int RootTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects().size();
}

int RootTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index].conditions.size();
}

FactPair RootTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const GlobalEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    const GlobalCondition &condition = effect.conditions[cond_index];
    return FactPair(condition.var, condition.val);
}

FactPair RootTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    const GlobalEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    return FactPair(effect.var, effect.val);
}

OperatorID RootTask::get_global_operator_id(OperatorID id) const {
    return id;
}

int RootTask::get_num_axioms() const {
    return g_axioms.size();
}

int RootTask::get_num_goals() const {
    return g_goal.size();
}

FactPair RootTask::get_goal_fact(int index) const {
    pair<int, int> &goal = g_goal[index];
    return FactPair(goal.first, goal.second);
}

vector<int> RootTask::get_initial_state_values() const {
    // TODO: think about a better way to do this.
    static StateRegistry state_registry(
        *g_root_task(), *g_state_packer, *g_axiom_evaluator, g_initial_state_data);
    return state_registry.get_initial_state().get_values();
}

void RootTask::convert_state_values(
    vector<int> &, const AbstractTask *ancestor_task) const {
    if (this != ancestor_task) {
        ABORT("Invalid state conversion");
    }
}

static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return g_root_task();
}

static PluginShared<AbstractTask> _plugin("no_transform", _parse);
}
