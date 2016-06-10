#include "transition_system.h"

#include "abstract_state.h"
#include "utils.h"

using namespace std;

namespace cegar {
TransitionSystem::TransitionSystem(const TaskProxy &task_proxy)
    : task_proxy(task_proxy) {
}

void TransitionSystem::add_loops_to_trivial_abstract_state(AbstractState *state) {
    for (OperatorProxy op : task_proxy.get_operators()) {
        add_loop(state, op.get_id());
    }
}

void TransitionSystem::add_arc(AbstractState *src, int op_id, AbstractState *target) {
    assert(src != target);
    src->add_outgoing_arc(op_id, target);
    target->add_incoming_arc(op_id, src);
}

void TransitionSystem::add_loop(AbstractState *state, int op_id) {
    state->add_loop(op_id);
}

void TransitionSystem::remove_incoming_arc(
    AbstractState *src, int op_id, AbstractState *target) {
    target->remove_incoming_arc(op_id, src);
}

void TransitionSystem::remove_outgoing_arc(
    AbstractState *src, int op_id, AbstractState *target) {
    src->remove_outgoing_arc(op_id, target);
}

void TransitionSystem::split_incoming_arcs(
    AbstractState *v, AbstractState *v1, AbstractState *v2, int var) {
    /* State v has been split into v1 and v2. Now for all transitions
       u->v we need to add transitions u->v1, u->v2, or both. */
    for (const Arc &arc : v->get_incoming_arcs()) {
        int op_id = arc.op_id;
        OperatorProxy op = task_proxy.get_operators()[op_id];
        AbstractState *u = arc.target;
        assert(u != v);
        int post = get_post(op, var);
        if (post == UNDEFINED_VALUE) {
            // op has no precondition and no effect on var.
            bool u_and_v1_intersect = u->domains_intersect(v1, var);
            if (u_and_v1_intersect) {
                add_arc(u, op_id, v1);
            }
            /* If the domains of u and v1 don't intersect, we must add
               the other arc and can avoid an intersection test. */
            if (!u_and_v1_intersect || u->domains_intersect(v2, var)) {
                add_arc(u, op_id, v2);
            }
        } else if (v1->contains(var, post)) {
            // op can only end in v1.
            add_arc(u, op_id, v1);
        } else {
            // op can only end in v2.
            assert(v2->contains(var, post));
            add_arc(u, op_id, v2);
        }
        remove_outgoing_arc(u, op_id, v);
    }
}

void TransitionSystem::split_outgoing_arcs(
    AbstractState *v, AbstractState *v1, AbstractState *v2, int var) {
    /* State v has been split into v1 and v2. Now for all transitions
       v->w we need to add transitions v1->w, v2->w, or both. */
    for (const Arc &arc : v->get_outgoing_arcs()) {
        int op_id = arc.op_id;
        OperatorProxy op = task_proxy.get_operators()[op_id];
        AbstractState *w = arc.target;
        assert(w != v);
        int pre = get_pre(op, var);
        int post = get_post(op, var);
        if (post == UNDEFINED_VALUE) {
            assert(pre == UNDEFINED_VALUE);
            // op has no precondition and no effect on var.
            bool v1_and_w_intersect = v1->domains_intersect(w, var);
            if (v1_and_w_intersect) {
                add_arc(v1, op_id, w);
            }
            /* If the domains of v1 and w don't intersect, we must add
               the other arc and can avoid an intersection test. */
            if (!v1_and_w_intersect || v2->domains_intersect(w, var)) {
                add_arc(v2, op_id, w);
            }
        } else if (pre == UNDEFINED_VALUE) {
            // op has no precondition, but an effect on var.
            add_arc(v1, op_id, w);
            add_arc(v2, op_id, w);
        } else if (v1->contains(var, pre)) {
            // op can only start in v1.
            add_arc(v1, op_id, w);
        } else {
            // op can only start in v2.
            assert(v2->contains(var, pre));
            add_arc(v2, op_id, w);
        }
        remove_incoming_arc(v, op_id, w);
    }
}

void TransitionSystem::split_loops(
    AbstractState *v, AbstractState *v1, AbstractState *v2, int var) {
    /* State v has been split into v1 and v2. Now for all self-loops
       v->v we need to add one or two of the transitions v1->v1, v1->v2,
       v2->v1 and v2->v2. */
    for (int op_id : v->get_loops()) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        int pre = get_pre(op, var);
        int post = get_post(op, var);
        if (pre == UNDEFINED_VALUE) {
            // op has no precondition on var --> it must start in v1 and v2.
            if (post == UNDEFINED_VALUE) {
                // op has no effect on var --> it must end in v1 and v2.
                add_loop(v1, op_id);
                add_loop(v2, op_id);
            } else if (v2->contains(var, post)) {
                // op must end in v2.
                add_arc(v1, op_id, v2);
                add_loop(v2, op_id);
            } else {
                // op must end in v1.
                assert(v1->contains(var, post));
                add_loop(v1, op_id);
                add_arc(v2, op_id, v1);
            }
        } else if (v1->contains(var, pre)) {
            // op must start in v1.
            assert(post != UNDEFINED_VALUE);
            if (v1->contains(var, post)) {
                // op must end in v1.
                add_loop(v1, op_id);
            } else {
                // op must end in v2.
                assert(v2->contains(var, post));
                add_arc(v1, op_id, v2);
            }
        } else {
            // op must start in v2.
            assert(v2->contains(var, pre));
            assert(post != UNDEFINED_VALUE);
            if (v1->contains(var, post)) {
                // op must end in v1.
                add_arc(v2, op_id, v1);
            } else {
                // op must end in v2.
                assert(v2->contains(var, post));
                add_loop(v2, op_id);
            }
        }
    }
}

void TransitionSystem::rewire(
    AbstractState *v, AbstractState *v1, AbstractState *v2, int var) {
    split_incoming_arcs(v, v1, v2, var);
    split_outgoing_arcs(v, v1, v2, var);
    split_loops(v, v1, v2, var);
}
}
