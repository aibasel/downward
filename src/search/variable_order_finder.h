#ifndef VARIABLE_ORDER_FINDER_H
#define VARIABLE_ORDER_FINDER_H

#include "task_proxy.h"

#include <memory>
#include <vector>

enum VariableOrderType {
    CG_GOAL_LEVEL,
    CG_GOAL_RANDOM,
    GOAL_CG_LEVEL,
    RANDOM,
    LEVEL,
    REVERSE_LEVEL
};

extern void dump_variable_order_type(VariableOrderType variable_order_type);

/*
  NOTE that the task proxy passed to the constructor is stored internally.
       It is the responsibility of the owner that the task lives at least as
       long as the variable order finder does.
*/
class VariableOrderFinder {
    TaskProxy task_proxy;
    const VariableOrderType variable_order_type;
    std::vector<int> selected_vars;
    std::vector<int> remaining_vars;
    std::vector<bool> is_goal_variable;
    std::vector<bool> is_causal_predecessor;

    void select_next(int position, int var_no);
public:
    VariableOrderFinder(const TaskProxy &task_proxy,
                        VariableOrderType variable_order_type);
    ~VariableOrderFinder() = default;
    bool done() const;
    int next();
};

#endif
