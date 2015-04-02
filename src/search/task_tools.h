#ifndef TASK_TOOLS_H
#define TASK_TOOLS_H

#include "task_proxy.h"

#include <utility>


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

/*
  Return true iff all operators have cost 1.

  Runtime: O(n), where n is the number of operators.
*/
bool is_unit_cost(TaskProxy task);

// Runtime: O(1)
bool has_axioms(TaskProxy task);

// Runtime: O(n), where n is the number of operators.
bool has_conditional_effects(TaskProxy task);

void dump_task(TaskProxy task);

inline std::pair<int, int> get_raw_fact(FactProxy fact) {
    return std::make_pair(fact.get_variable().get_id(), fact.get_value());
}

#endif
