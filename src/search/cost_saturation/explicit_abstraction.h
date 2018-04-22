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

std::ostream &operator<<(std::ostream &os, const Successor &transition);

class ExplicitAbstraction : public Abstraction {
    const AbstractionFunction abstraction_function;

    // State-changing transitions.
    std::vector<std::vector<Successor>> backward_graph;

    mutable priority_queues::AdaptiveQueue<int> queue;

protected:
    virtual std::vector<int> compute_saturated_costs(
        const std::vector<int> &h_values,
        int num_operators,
        bool use_general_costs) const override;

public:
    ExplicitAbstraction(
        AbstractionFunction function,
        std::vector<std::vector<Successor>> &&backward_graph,
        std::vector<int> &&looping_operators,
        std::vector<int> &&goal_states);

    virtual std::vector<int> compute_h_values(
        const std::vector<int> &costs) const override;

    virtual std::vector<Transition> get_transitions() const override;

    virtual int get_num_states() const override;

    virtual int get_abstract_state_id(const State &concrete_state) const override;

    virtual void remove_transition_system() override;

    virtual void dump() const override;
};
}

#endif
