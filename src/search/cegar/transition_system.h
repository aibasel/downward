#ifndef CEGAR_TRANSITION_SYSTEM_H
#define CEGAR_TRANSITION_SYSTEM_H

#include "../task_proxy.h"

namespace cegar {
class AbstractState;

class TransitionSystem {
    const std::shared_ptr<AbstractTask> task;

    int num_non_loops;
    int num_loops;

    TaskProxy get_task_proxy() const;

    void add_transition(AbstractState *src, int op_id, AbstractState *target);
    void add_loop(AbstractState *state, int op_id);

    void remove_incoming_transition(
        AbstractState *src, int op_id, AbstractState *target);
    void remove_outgoing_transition(
        AbstractState *src, int op_id, AbstractState *target);

    void split_incoming_transitions(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
    void split_outgoing_transitions(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
    void split_loops(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);

public:
    explicit TransitionSystem(const std::shared_ptr<AbstractTask> &task);

    void add_loops_to_trivial_abstract_state(AbstractState *state);

    // Update transition system after v has been split into v1 and v2.
    void rewire(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);

    int get_num_non_loops() const;
    int get_num_loops() const;
};
}

#endif
