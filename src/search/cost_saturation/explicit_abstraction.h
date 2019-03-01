#ifndef COST_SATURATION_EXPLICIT_ABSTRACTION_H
#define COST_SATURATION_EXPLICIT_ABSTRACTION_H

#include "abstraction.h"

#include "../algorithms/priority_queues.h"

#include <functional>
#include <limits>
#include <utility>
#include <vector>

class State;

namespace cost_saturation {
using AbstractionFunction = std::function<int (const State &)>;

struct Successor {
    int op;
    int state;

    Successor(int op, int state)
        : op(op),
          state(state) {
    }

    bool operator<(const Successor &other) const {
        return std::make_pair(op, state) < std::make_pair(other.op, other.state);
    }

    bool operator>=(const Successor &other) const {
        return !(*this < other);
    }
};

std::ostream &operator<<(std::ostream &os, const Successor &successor);


class ExplicitAbstraction : public Abstraction {
    const AbstractionFunction abstraction_function;

    // State-changing transitions.
    std::vector<std::vector<Successor>> backward_graph;

    // Operators inducing state-changing transitions.
    std::vector<bool> active_operators;

    // Operators inducing self-loops.
    std::vector<bool> looping_operators;

    std::vector<int> goal_states;

    mutable priority_queues::AdaptiveQueue<int> queue;

protected:
    virtual void release_transition_system_memory() override;

public:
    ExplicitAbstraction(
        AbstractionFunction function,
        std::vector<std::vector<Successor>> &&backward_graph,
        std::vector<bool> &&looping_operators,
        std::vector<int> &&goal_states);

    virtual std::vector<int> compute_goal_distances(
        const std::vector<int> &costs) const override;
    virtual std::vector<int> compute_saturated_costs(
        const std::vector<int> &h_values) const override;
    virtual int get_num_states() const override;
    virtual int get_abstract_state_id(const State &concrete_state) const override;
    virtual bool operator_is_active(int op_id) const override;
    virtual bool operator_induces_self_loop(int op_id) const override;
    virtual const std::vector<int> &get_goal_states() const override;
    virtual void dump() const override;
};
}

#endif
