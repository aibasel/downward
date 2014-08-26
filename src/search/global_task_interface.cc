#include "global_task_interface.h"

#include "option_parser.h"
#include "plugin.h"

using namespace std;

class Task;

int GlobalTaskInterface::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_preconditions().size();
}

pair<int, int> GlobalTaskInterface::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    assert(fact_index < get_num_operator_preconditions(op_index, is_axiom));
    const Operator &op = get_operator_or_axiom(op_index, is_axiom);
    const GlobalOperatorCondition &precondition = op.get_preconditions()[fact_index];
    assert(precondition.var >= 0 && precondition.var < g_variable_domain.size());
    assert(precondition.value >= 0 && precondition.value < g_variable_domain[precondition.var]);
    return make_pair(precondition.var, precondition.value);
}

int GlobalTaskInterface::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).get_effects().size();
}

int GlobalTaskInterface::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    assert(eff_index < get_num_operator_effects(op_index, is_axiom));
    return get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index].conditions.size();
}

pair<int, int> GlobalTaskInterface::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    assert(eff_index < get_num_operator_effects(op_index, is_axiom));
    const GlobalOperatorEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    assert(cond_index < effect.conditions.size());
    const GlobalOperatorCondition &condition = effect.conditions[cond_index];
    assert(condition.var >= 0 && condition.var < g_variable_domain.size());
    assert(condition.value >= 0 && condition.value < g_variable_domain[condition.var]);
    return make_pair(condition.var, condition.value);
}

pair<int, int> GlobalTaskInterface::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    assert(eff_index < get_num_operator_effects(op_index, is_axiom));
    const GlobalOperatorEffect &effect = get_operator_or_axiom(op_index, is_axiom).get_effects()[eff_index];
    assert(effect.var >= 0 && effect.var < g_variable_domain.size());
    assert(effect.value >= 0 && effect.value < g_variable_domain[effect.var]);
    return make_pair(effect.var, effect.value);
}

const Operator *GlobalTaskInterface::get_global_operator(int index, bool is_axiom) const {
    return &get_operator_or_axiom(index, is_axiom);
}

static Task *_parse(OptionParser &parser) {
    if (parser.dry_run())
        return 0;
    else
        return new Task(new GlobalTaskInterface());
}

static Plugin<Task> _plugin("global_task", _parse);
