#include "global_task.h"

#include "state_id.h"
#include "state_registry.h"

using namespace std;

GlobalTaskInterface::GlobalTaskInterface() {
}

GlobalTaskInterface::~GlobalTaskInterface() {
}

size_t GlobalTaskInterface::get_state_value(size_t state_id, size_t var) const {
    assert(state_id < g_state_registry->size());
    return g_state_registry->lookup_state(StateID(state_id))[var];
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

pair<size_t, size_t> GlobalTaskInterface::get_operator_precondition_fact(size_t op_index, size_t fact_index) const {
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
