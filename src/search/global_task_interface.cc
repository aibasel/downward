#include "global_task_interface.h"

#include "option_parser.h"
#include "plugin.h"

using namespace std;

size_t GlobalTaskInterface::get_num_operator_preconditions(size_t index) const {
    assert(index < g_operators.size());
    return g_operators[index].get_preconditions().size();
}

pair<size_t, size_t> GlobalTaskInterface::get_operator_precondition(
    size_t op_index, size_t fact_index) const {
    assert(op_index < g_operators.size());
    assert(fact_index < get_num_operator_preconditions(op_index));
    const Operator &op = g_operators[op_index];
    const OperatorCondition &precondition = op.get_preconditions()[fact_index];
    assert(precondition.var >= 0 && precondition.var < g_variable_domain.size());
    assert(precondition.value >= 0 && precondition.value < g_variable_domain[precondition.var]);
    return make_pair(precondition.var, precondition.value);
}

pair<size_t, size_t> GlobalTaskInterface::get_operator_effect_condition(
    size_t op_index, size_t eff_index, size_t cond_index) const {
    assert(op_index < g_operators.size());
    assert(eff_index < g_operators[op_index].get_pre_post().size());
    const PrePost &pre_post = g_operators[op_index].get_pre_post()[eff_index];
    assert(cond_index < pre_post.cond.size());
    const Prevail &condition = pre_post.cond[cond_index];
    assert(condition.var >= 0 && condition.var < g_variable_domain.size());
    assert(condition.prev >= 0 && condition.prev < g_variable_domain[condition.var]);
    return make_pair(condition.var, condition.prev);
}

pair<size_t, size_t> GlobalTaskInterface::get_operator_effect(
    size_t op_index, size_t eff_index) const {
    assert(op_index < g_operators.size());
    assert(eff_index < g_operators[op_index].get_pre_post().size());
    const PrePost &pre_post = g_operators[op_index].get_pre_post()[eff_index];
    assert(pre_post.var >= 0 && pre_post.var < g_variable_domain.size());
    assert(pre_post.post >= 0 && pre_post.post < g_variable_domain[pre_post.var]);
    return make_pair(pre_post.var, pre_post.post);
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
