#ifndef PARTIALLY_RELAXED_STATE_H
#define PARTIALLY_RELAXED_STATE_H

#include <vector>

class Operator;
class PartialRelaxation;
class State;

class PartiallyRelaxedState {
    // TODO: Carrying the relaxation around with each state might be a
    //       bit too much overhead, but it makes the code cleaner for
    //       now. In the future, this may be part of a "state space"
    //       abstraction which could be passed into the relevant
    //       methods.
    const PartialRelaxation *relaxation;

    std::vector<bool> relaxed_vars;
    std::vector<int> unrelaxed_vars;
public:
    explicit PartiallyRelaxedState(const PartialRelaxation &relaxation, const State &state);
    PartiallyRelaxedState(const PartiallyRelaxedState &predecessor, const Operator &op);

    void set(int var_no, int value);
    int get(int var_no) const; // unrelaxed variables only
    bool has_value(int var_no, int value) const;

    void dump() const;
    bool operator<(const PartiallyRelaxedState &other) const;

    const PartialRelaxation &get_relaxation() const;
};

#endif
