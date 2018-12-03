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
    virtual std::vector<int> compute_saturated_costs(
        const std::vector<int> &h_values,
        int num_operators) const = 0;

    virtual void release_transition_system_memory() = 0;

    bool has_transition_system() const;

public:
    Abstraction();
    virtual ~Abstraction() = default;

    Abstraction(const Abstraction &) = delete;

    virtual int get_abstract_state_id(const State &concrete_state) const = 0;

    virtual std::vector<int> compute_goal_distances(
        const std::vector<int> &costs) const = 0;

    std::pair<std::vector<int>, std::vector<int>>
    compute_goal_distances_and_saturated_costs(
        const std::vector<int> &costs) const;

    // Operators inducing state-changing transitions.
    virtual const std::vector<int> &get_active_operators() const = 0;

    // Operators inducing self-looping transitions. May overlap with active
    // operators.
    virtual const std::vector<int> &get_looping_operators() const = 0;

    virtual int get_num_states() const = 0;
    virtual const std::vector<int> &get_goal_states() const = 0;

    void remove_transition_system();

    virtual void dump() const = 0;
};
}

#endif
