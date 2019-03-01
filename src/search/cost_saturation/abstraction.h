#ifndef COST_SATURATION_ABSTRACTION_H
#define COST_SATURATION_ABSTRACTION_H

#include <cassert>
#include <ostream>
#include <vector>

class State;

namespace cost_saturation {
struct Transition {
    int src;
    int op;
    int target;

    Transition(int src, int op, int target)
        : src(src),
          op(op),
          target(target) {
        assert(src != target);
    }

    bool operator==(const Transition &other) const {
        return src == other.src && op == other.op && target == other.target;
    }

    bool operator<(const Transition &other) const {
        return src < other.src
               || (src == other.src && op < other.op)
               || (src == other.src && op == other.op && target < other.target);
    }

    friend std::ostream &operator<<(std::ostream &os, const Transition &t) {
        return os << "<" << t.src << "," << t.op << "," << t.target << ">";
    }
};


class Abstraction {
    bool has_transition_system_;

protected:
    virtual void release_transition_system_memory() = 0;

    bool has_transition_system() const;

public:
    Abstraction();
    virtual ~Abstraction() = default;

    Abstraction(const Abstraction &) = delete;

    virtual int get_abstract_state_id(const State &concrete_state) const = 0;

    virtual std::vector<int> compute_goal_distances(
        const std::vector<int> &costs) const = 0;
    virtual std::vector<int> compute_saturated_costs(
        const std::vector<int> &h_values) const = 0;

    // Return true iff operator induced a state-changing transition.
    virtual bool operator_is_active(int op_id) const = 0;

    // Return true iff operator induced a self-loop. Note that an operator may
    // induce both state-changing transitions and self-loops.
    virtual bool operator_induces_self_loop(int op_id) const = 0;

    virtual int get_num_states() const = 0;
    virtual const std::vector<int> &get_goal_states() const = 0;

    void remove_transition_system();

    virtual void dump() const = 0;
};
}

#endif
