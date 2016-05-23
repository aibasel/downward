#ifndef TASK_TOOLS_H
#define TASK_TOOLS_H

#include "task_proxy.h"


inline bool is_applicable(const OperatorProxy &op, const State &state) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (state[precondition.get_variable()] != precondition)
            return false;
    }
    return true;
}

inline bool is_goal_state(const TaskProxy &task_proxy, const State &state) {
    for (FactProxy goal : task_proxy.get_goals()) {
        if (state[goal.get_variable()] != goal)
            return false;
    }
    return true;
}

/*
  Return true iff all operators have cost 1.

  Runtime: O(n), where n is the number of operators.
*/
bool is_unit_cost(const TaskProxy &task_proxy);

// Runtime: O(1)
bool has_axioms(const TaskProxy &task_proxy);

/*
  Report an error and exit with ExitCode::UNSUPPORTED if the task has axioms.
  Runtime: O(1)
*/
void verify_no_axioms(const TaskProxy &task_proxy);

// Runtime: O(n), where n is the number of operators.
bool has_conditional_effects(const TaskProxy &task_proxy);

/*
  Report an error and exit with ExitCode::UNSUPPORTED if the task has
  conditional effects.
  Runtime: O(n), where n is the number of operators.
*/
void verify_no_conditional_effects(const TaskProxy &task_proxy);

double get_average_operator_cost(const TaskProxy &task_proxy);

#endif
