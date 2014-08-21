#include "global_task_interface.h"

#include "option_parser.h"
#include "plugin.h"
#include "state_id.h"
#include "state_registry.h"

using namespace std;

size_t GlobalTaskInterface::get_num_operator_preconditions(size_t index) const {
    assert(index < g_operators.size());
    return g_operators[index].get_prevail().size() +
        g_operators[index].get_indices_of_effects_with_precondition().size();
}

pair<size_t, size_t> GlobalTaskInterface::get_operator_precondition(
    size_t op_index, size_t fact_index) const {
    assert(op_index < g_operators.size());
    assert(fact_index < get_num_operator_preconditions(op_index));
    const Operator &op = g_operators[op_index];
    size_t num_prevails = op.get_prevail().size();
    if (fact_index < num_prevails) {
        const Prevail &prevail = op.get_prevail()[fact_index];
        assert(prevail.var >= 0 && prevail.var < g_variable_domain.size());
        assert(prevail.prev >= 0 && prevail.prev < g_variable_domain[prevail.var]);
        return make_pair(prevail.var, prevail.prev);
    }
    size_t pre_post_index = op.get_indices_of_effects_with_precondition()[fact_index - num_prevails];
    const PrePost &pre_post = op.get_pre_post()[pre_post_index];
    assert(pre_post.var >= 0 && pre_post.var < g_variable_domain.size());
    assert(pre_post.pre >= 0 && pre_post.pre < g_variable_domain[pre_post.var]);
    return make_pair(pre_post.var, pre_post.pre);
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
