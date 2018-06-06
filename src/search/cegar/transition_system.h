#ifndef CEGAR_TRANSITION_SYSTEM_H
#define CEGAR_TRANSITION_SYSTEM_H

#include "abstract_state.h"
#include "transition.h"
#include "types.h"

#include "../utils/collections.h"

#include <memory>
#include <vector>

struct FactPair;
class OperatorsProxy;

namespace cegar {
class AbstractState;

/*
  Rewire transitions after each split.
*/
class TransitionSystem {
    const std::vector<std::vector<FactPair>> preconditions_by_operator;
    const std::vector<std::vector<FactPair>> effects_by_operator;
    const std::vector<std::vector<FactPair>> postconditions_by_operator;

    // Transitions from and to other abstract states.
    std::vector<Transitions> incoming;
    std::vector<Transitions> outgoing;

    // Store self-loops (operator indices) separately to save space.
    std::vector<Loops> loops;

    int num_non_loops;
    int num_loops;

    void enlarge_vectors_by_one();

    void add_incoming_transition(int src_id, int op_id, int target_id);
    void add_outgoing_transition(int src_id, int op_id, int target_id);
    void add_transition_both_ways(int src_id, int op_id, int target_id);
    void add_loop(int state_id, int op_id);

    void rewire_incoming_transitions(
        const Transitions &old_incoming, const AbstractStates &states,
        AbstractState *v1, AbstractState *v2, int var);
    void rewire_outgoing_transitions(
        const Transitions &old_outgoing, const AbstractStates &states,
        AbstractState *v1, AbstractState *v2, int var);
    void rewire_loops(
        const Loops &old_loops, AbstractState *v1, AbstractState *v2, int var);

public:
    explicit TransitionSystem(const OperatorsProxy &ops);

    void initialize(AbstractState *initial_state);

    // Update transition system after v has been split into v1 and v2.
    void rewire(
        const AbstractStates &states, AbstractState *v, AbstractState *v1, AbstractState *v2, int var);

    const Transitions &get_incoming_transitions(int state_id) const {
        assert(utils::in_bounds(state_id, incoming));
        return incoming[state_id];
    }

    const std::vector<Transitions> &get_incoming_transitions() const {
        return incoming;
    }

    const Transitions &get_outgoing_transitions(int state_id) const {
        assert(utils::in_bounds(state_id, outgoing));
        return outgoing[state_id];
    }

    const Transitions &get_outgoing_transitions(AbstractState *state) const {
        return get_outgoing_transitions(state->get_id());
    }

    const std::vector<Transitions> &get_outgoing_transitions() const {
        return outgoing;
    }

    const Loops &get_loops(int state_id) const {
        assert(utils::in_bounds(state_id, loops));
        return loops[state_id];
    }

    // TODO: These methods should probably live in a separate class.
    int get_precondition_value(int op_id, int var) const;
    int get_effect_value(int op_id, int var) const;
    int get_postcondition_value(int op_id, int var) const;

    const std::vector<FactPair> get_preconditions(int op_id) const {
        assert(utils::in_bounds(op_id, preconditions_by_operator));
        return preconditions_by_operator[op_id];
    }

    const std::vector<FactPair> get_effects(int op_id) const {
        assert(utils::in_bounds(op_id, effects_by_operator));
        return effects_by_operator[op_id];
    }

    int get_num_states() const;
    int get_num_operators() const;
    int get_num_non_loops() const;
    int get_num_loops() const;

    std::vector<int> extract_operators_inducing_loops();

    std::vector<Transitions> extract_incoming_transitions();
    std::vector<Transitions> extract_outgoing_transitions();

    void print_statistics() const;
};
}

#endif
