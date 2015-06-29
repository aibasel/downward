#include "root_task.h"

#include "global_operator.h"
#include "globals.h"
#include "option_parser.h"
#include "plugin.h"

#include <cassert>

using namespace std;


static GlobalOperator &get_operator_or_axiom(int index, bool is_axiom) {
    if (is_axiom) {
        assert(in_bounds(index, g_axioms));
        return g_axioms[index];
    } else {
        assert(in_bounds(index, g_operators));
        return g_operators[index];
    }
}

int RootTask::get_num_variables() const {
    return g_variable_domain.size();
}

const string &RootTask::get_variable_name(int var) const {
    return g_variable_name[var];
}

int RootTask::get_variable_domain_size(int var) const {
    return g_variable_domain[var];
}

const string &RootTask::get_fact_name(int var, int value) const {
    return g_fact_names[var][value];
}

int RootTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_cost();
}

const string &RootTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_name();
}

int RootTask::get_num_operators() const {
    return g_operators.size();
}

int RootTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_preconditions().size();
}

pair<int, int> RootTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const GlobalOperator &op = get_operator_or_axiom(op_index, is_axiom);
    const GlobalCondition &precondition = op.get_preconditions()[fact_index];
    return make_pair(precondition.var, precondition.val);
}

int RootTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects().size();
}

int RootTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index].conditions.size();
}

pair<int, int> RootTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const GlobalEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    const GlobalCondition &condition = effect.conditions[cond_index];
    return make_pair(condition.var, condition.val);
}

pair<int, int> RootTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    const GlobalEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    return make_pair(effect.var, effect.val);
}

const GlobalOperator *RootTask::get_global_operator(int index, bool is_axiom) const {
    return &get_operator_or_axiom(index, is_axiom);
}

int RootTask::get_num_axioms() const {
    return g_axioms.size();
}

int RootTask::get_num_goals() const {
    return g_goal.size();
}

pair<int, int> RootTask::get_goal_fact(int index) const {
    return g_goal[index];
}

vector<int> RootTask::get_initial_state_values() const {
    return get_state_values(g_initial_state());
}

vector<int> RootTask::get_state_values(const GlobalState &global_state) const {
    // TODO: Use unpacked values directly once issue348 is merged.
    int num_vars = g_variable_domain.size();
    vector<int> values(num_vars);
    for (int var = 0; var < num_vars; ++var)
        values[var] = global_state[var];
    return values;
}

static shared_ptr<AbstractTask> _parse(OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return g_root_task();
}

static PluginShared<AbstractTask> _plugin("no_transform", _parse);
