#ifndef TASK_TOOLS_H
#define TASK_TOOLS_H

#include "task_proxy.h"


inline bool is_applicable(OperatorProxy op, const State &state) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (state[precondition.get_variable()] != precondition)
            return false;
    }
    return true;
}

inline bool is_goal_state(TaskProxy task, const State &state) {
    for (FactProxy goal : task.get_goals()) {
        if (state[goal.get_variable()] != goal)
            return false;
    }
    return true;
}

inline bool is_unit_cost(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        if (op.get_cost() != 1)
            return false;
    }
    return true;
}

inline bool has_axioms(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        if (op.is_axiom())
            return true;
    }
    return false;
}

inline bool has_conditional_effects(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        for (EffectProxy effect : op.get_effects()) {
            if (!effect.get_conditions().empty())
                return true;
        }
    }
    return false;
}

#endif
