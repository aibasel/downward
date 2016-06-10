#ifndef CEGAR_TRANSITION_SYSTEM_H
#define CEGAR_TRANSITION_SYSTEM_H

#include "../task_proxy.h"

namespace cegar {
class AbstractState;

class TransitionSystem {
    const TaskProxy &task_proxy;

    void add_arc(AbstractState *src, int op_id, AbstractState *target);
    void add_loop(AbstractState *state, int op_id);

    void remove_incoming_arc(
        AbstractState *src, int op_id, AbstractState *target);
    void remove_outgoing_arc(
        AbstractState *src, int op_id, AbstractState *target);

    void split_incoming_arcs(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
    void split_outgoing_arcs(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
    void split_loops(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);

public:
    TransitionSystem(const TaskProxy &task_proxy);
    ~TransitionSystem() = default;

    void add_loops_to_trivial_abstract_state(AbstractState *state);

    // Update transition system after v has been split into v1 and v2.
    void rewire(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
};
}

#endif
