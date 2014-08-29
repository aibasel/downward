#ifndef OPERATOR_H
#define OPERATOR_H

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "globals.h"
#include "state.h"

struct Condition {
    int var;
    int val;
    explicit Condition(std::istream &in);
    Condition(int variable, int value) : var(variable), val(value) {
        assert(var >= 0 && var < g_variable_name.size());
        assert(val >= 0 && val < g_variable_domain[var]);
    }

    bool is_applicable(const State &state) const {
        return state[var] == val;
    }

    bool operator==(const Condition &other) const {
        return var == other.var && val == other.val;
    }

    bool operator!=(const Condition &other) const {
        return !(*this == other);
    }

    void dump() const;
};

struct Effect {
    int var;
    int val;
    std::vector<Condition> conditions;
    explicit Effect(std::istream &in);
    Effect(int variable, int value, const std::vector<Condition> &conds)
        : var(variable), val(value), conditions(conds) {}

    bool does_fire(const State &state) const {
        for (int i = 0; i < conditions.size(); ++i)
            if (!conditions[i].is_applicable(state))
                return false;
        return true;
    }

    void dump() const;
};

class Operator {
    bool is_an_axiom;
    std::vector<Condition> preconditions;
    std::vector<Effect> effects;
    std::string name;
    int cost;

    mutable bool marked; // Used for short-term marking of preferred operators
    void read_pre_post(std::istream &in);
public:
    explicit Operator(std::istream &in, bool is_axiom);
    void dump() const;
    std::string get_name() const {return name; }

    bool is_axiom() const {return is_an_axiom; }

    const std::vector<Condition> &get_preconditions() const {return preconditions; }
    const std::vector<Effect> &get_effects() const {return effects; }

    bool is_applicable(const State &state) const {
        for (int i = 0; i < preconditions.size(); ++i)
            if (!preconditions[i].is_applicable(state))
                return false;
        return true;
    }

    bool is_marked() const {
        return marked;
    }
    void mark() const {
        marked = true;
    }
    void unmark() const {
        marked = false;
    }

    mutable bool marker1, marker2; // HACK! HACK!

    int get_cost() const {return cost; }
};

#endif
