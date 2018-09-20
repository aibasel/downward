#ifndef CEGAR_TRANSITION_UPDATER_H
#define CEGAR_TRANSITION_UPDATER_H

struct FactPair;
class OperatorsProxy;

#include <memory>
#include <vector>

namespace cegar {
class AbstractState;

/*
  Rewire transitions after each split.
*/
class TransitionUpdater {
    std::vector<std::vector<FactPair>> preconditions_by_operator;
    std::vector<std::vector<FactPair>> postconditions_by_operator;

    int num_non_loops;
    int num_loops;

    int get_precondition_value(int op_id, int var) const;
    int get_postcondition_value(int op_id, int var) const;

    void add_transition(AbstractState *src, int op_id, AbstractState *target);
    void add_loop(AbstractState *state, int op_id);

    void remove_incoming_transition(
        AbstractState *src, int op_id, AbstractState *target);
    void remove_outgoing_transition(
        AbstractState *src, int op_id, AbstractState *target);

    void rewire_incoming_transitions(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
    void rewire_outgoing_transitions(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);
    void rewire_loops(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);

public:
    explicit TransitionUpdater(const OperatorsProxy &ops);

    void add_loops_to_trivial_abstract_state(AbstractState *state);

    // Update transition system after v has been split into v1 and v2.
    void rewire(
        AbstractState *v, AbstractState *v1, AbstractState *v2, int var);

    int get_num_non_loops() const;
    int get_num_loops() const;
};
}

#endif
