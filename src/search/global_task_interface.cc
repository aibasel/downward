#include "global_task_interface.h"

#include "option_parser.h"
#include "plugin.h"

using namespace std;

size_t GlobalTaskInterface::get_num_operator_preconditions(size_t index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).get_preconditions().size();
}

pair<size_t, size_t> GlobalTaskInterface::get_operator_precondition(
    size_t op_index, size_t fact_index, bool is_axiom) const {
    assert(op_index < g_operators.size());
    assert(fact_index < get_num_operator_preconditions(op_index, is_axiom));
    const Operator &op = g_operators[op_index];
    const OperatorCondition &precondition = op.get_preconditions()[fact_index];
    assert(precondition.var >= 0 && precondition.var < g_variable_domain.size());
    assert(precondition.value >= 0 && precondition.value < g_variable_domain[precondition.var]);
    return make_pair(precondition.var, precondition.value);
}

size_t GlobalTaskInterface::get_num_operator_effects(std::size_t op_index) const {
    assert(op_index < g_operators.size());
    return g_operators[op_index].get_effects().size();
}

size_t GlobalTaskInterface::get_num_operator_effect_conditions(
    std::size_t op_index, std::size_t eff_index) const {
    assert(op_index < g_operators.size());
    assert(eff_index < get_num_operator_effects(op_index));
    return g_operators[op_index].get_effects()[eff_index].conditions.size();
}

pair<size_t, size_t> GlobalTaskInterface::get_operator_effect_condition(
    size_t op_index, size_t eff_index, size_t cond_index) const {
    assert(op_index < g_operators.size());
    assert(eff_index < get_num_operator_effects(op_index));
    const OperatorEffect &effect = g_operators[op_index].get_effects()[eff_index];
    assert(cond_index < effect.conditions.size());
    const OperatorCondition &condition = effect.conditions[cond_index];
    assert(condition.var >= 0 && condition.var < g_variable_domain.size());
    assert(condition.value >= 0 && condition.value < g_variable_domain[condition.var]);
    return make_pair(condition.var, condition.value);
}

pair<size_t, size_t> GlobalTaskInterface::get_operator_effect(
    size_t op_index, size_t eff_index) const {
    assert(op_index < g_operators.size());
    assert(eff_index < get_num_operator_effects(op_index));
    const OperatorEffect &effect = g_operators[op_index].get_effects()[eff_index];
    assert(effect.var >= 0 && effect.var < g_variable_domain.size());
    assert(effect.value >= 0 && effect.value < g_variable_domain[effect.var]);
    return make_pair(effect.var, effect.value);
}

const Operator *GlobalTaskInterface::get_original_operator(size_t index) const {
    assert(index < g_operators.size());
    return &g_operators[index];
}

static TaskInterface *_parse(OptionParser &parser) {
    if (parser.dry_run())
        return 0;
    else
        return new GlobalTaskInterface();
}

static Plugin<TaskInterface> _plugin("global_task_interface", _parse);
