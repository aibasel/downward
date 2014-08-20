#include "global_task.h"

#include "state_id.h"
#include "state_registry.h"

using namespace std;

GlobalTaskInterface::GlobalTaskInterface() {
}

GlobalTaskInterface::~GlobalTaskInterface() {
}

size_t GlobalTaskInterface::get_state_value(size_t state_index, size_t var) const {
    assert(state_index < g_state_registry->size());
    return g_state_registry->lookup_state(StateID(state_index))[var];
}

size_t GlobalTaskInterface::get_operator_precondition_size(size_t index) const {
    assert(index < g_operators.size());
    size_t size = g_operators[index].get_prevail().size();
    for (int i = 0; i < g_operators[index].get_pre_post().size(); ++i) {
        if (g_operators[index].get_pre_post()[i].pre != -1)
            ++size;
    }
    return size;
}

pair<size_t, size_t> GlobalTaskInterface::get_operator_precondition_fact(
    size_t op_index, size_t fact_index) const {
    assert(op_index < g_operators.size());
    assert(fact_index < get_operator_precondition_size(op_index));
    size_t num_prevails = g_operators[op_index].get_prevail().size();
    if (fact_index < num_prevails) {
        const Prevail &prevail = g_operators[op_index].get_prevail()[fact_index];
        assert(prevail.var >= 0 && prevail.var < g_variable_domain.size());
        assert(prevail.prev >= 0 && prevail.prev < g_variable_domain[prevail.var]);
        return make_pair(prevail.var, prevail.prev);
    }
    size_t seen_preconditions = num_prevails;
    size_t pre_post_index = -1;
    for (int i = 0; i < g_operators[op_index].get_pre_post().size(); ++i) {
        if (g_operators[op_index].get_pre_post()[i].pre != -1) {
            ++seen_preconditions;
            if (seen_preconditions - 1 == fact_index) {
                pre_post_index = i;
                break;
            }
        }
    }
    assert(pre_post_index != -1);
    const PrePost &pre_post = g_operators[op_index].get_pre_post()[pre_post_index];
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

bool GlobalTaskInterface::operator_is_applicable_in_state(
    size_t op_index, size_t state_index) const {
    const State &state = g_state_registry->lookup_state(StateID(state_index));
    return g_operators[op_index].is_applicable(state);
}

const Operator &GlobalTaskInterface::get_original_operator(size_t index) const {
    assert(index < g_operators.size());
    return g_operators[index];
}
