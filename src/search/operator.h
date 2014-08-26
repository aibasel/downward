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
    Condition(std::istream &in);
    Condition(int v, int p) : var(v), val(p) {}

    bool is_applicable(const State &state) const {
        assert(var >= 0 && var < g_variable_name.size());
        assert(val >= 0 && val < g_variable_domain[var]);
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
    Effect(std::istream &in);
    Effect(int var, int val, const std::vector<Condition> &co)
        : var(var), val(val), conditions(co) {}

    // TODO do we need to preserve these assertions?
    bool is_applicable(const State &) const {
        assert(var >= 0 && var < g_variable_name.size());
        assert(pre == -1 || (pre >= 0 && pre < g_variable_domain[var]));
        return true;
    }

    bool does_fire(const State &state) const {
        for (int i = 0; i < conditions.size(); i++)
            if (!conditions[i].is_applicable(state))
                return false;
        return true;
    }

    void dump() const;
};

class Operator {
    bool is_an_axiom;
    std::vector<Condition> preconditions;      // var, val
    std::vector<Effect> effects;     // var, val, effect conditions
    std::string name;
    int cost;

    mutable bool marked; // Used for short-term marking of preferred operators
    void read_pre_post(std::istream &in);
public:
    Operator(std::istream &in, bool is_axiom);
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

//class OldOperator {
//    bool is_an_axiom;
//    std::vector<Prevail> prevail;      // var, val
//    std::vector<PrePost> pre_post;     // var, old-val, new-val, effect conditions
//    std::string name;
//    int cost;
//
//    mutable bool marked; // Used for short-term marking of preferred operators
//public:
//    Operator(std::istream &in, bool is_axiom);
//    void dump() const;
//    std::string get_name() const {return name; }
//
//    bool is_axiom() const {return is_an_axiom; }
//
//    const std::vector<Prevail> &get_prevail() const {return prevail; }
//    const std::vector<PrePost> &get_pre_post() const {return pre_post; }
//
//    bool is_applicable(const State &state) const {
//        for (int i = 0; i < prevail.size(); i++)
//            if (!prevail[i].is_applicable(state))
//                return false;
//        for (int i = 0; i < pre_post.size(); i++)
//            if (!pre_post[i].is_applicable(state))
//                return false;
//        return true;
//    }
//
//    bool is_marked() const {
//        return marked;
//    }
//    void mark() const {
//        marked = true;
//    }
//    void unmark() const {
//        marked = false;
//    }
//
//    mutable bool marker1, marker2; // HACK! HACK!
//
//    int get_cost() const {return cost; }
//};

#endif
