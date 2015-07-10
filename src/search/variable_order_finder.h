#ifndef VARIABLE_ORDER_FINDER_H
#define VARIABLE_ORDER_FINDER_H

#include <memory>
#include <vector>

class TaskProxy;

enum VariableOrderType {
    CG_GOAL_LEVEL,
    CG_GOAL_RANDOM,
    GOAL_CG_LEVEL,
    RANDOM,
    LEVEL,
    REVERSE_LEVEL
};

class VariableOrderFinder {
    const TaskProxy &task_proxy;
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
    void dump() const;
};

#endif
