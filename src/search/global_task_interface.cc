#include "global_task_interface.h"

#include "global_operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "task.h"

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

int GlobalTaskInterface::get_num_variables() const {
    return g_variable_domain.size();
}

int GlobalTaskInterface::get_variable_domain_size(int var) const {
    return g_variable_domain[var];
}

int GlobalTaskInterface::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_cost();
}

const std::string &GlobalTaskInterface::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_name();
}

int GlobalTaskInterface::get_num_operators() const {
    return g_operators.size();
}

int GlobalTaskInterface::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_preconditions().size();
}

pair<int, int> GlobalTaskInterface::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const GlobalOperator &op = get_operator_or_axiom(op_index, is_axiom);
    const GlobalCondition &precondition = op.get_preconditions()[fact_index];
    return make_pair(precondition.var, precondition.val);
}

int GlobalTaskInterface::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects().size();
}

int GlobalTaskInterface::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index].conditions.size();
}

pair<int, int> GlobalTaskInterface::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const GlobalEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    const GlobalCondition &condition = effect.conditions[cond_index];
    return make_pair(condition.var, condition.val);
}

pair<int, int> GlobalTaskInterface::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    const GlobalEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    return make_pair(effect.var, effect.val);
}

const GlobalOperator *GlobalTaskInterface::get_global_operator(int index, bool is_axiom) const {
    return &get_operator_or_axiom(index, is_axiom);
}

int GlobalTaskInterface::get_num_axioms() const {
    return g_axioms.size();
}

int GlobalTaskInterface::get_num_goals() const {
    return g_goal.size();
}

std::pair<int, int> GlobalTaskInterface::get_goal_fact(int index) const {
    return g_goal[index];
}

static Task *_parse(OptionParser &parser) {
    if (parser.dry_run())
        return 0;
    else
        return new Task(new GlobalTaskInterface());
}

static Plugin<Task> _plugin("global_task", _parse);
