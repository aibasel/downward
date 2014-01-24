#ifndef MERGE_AND_SHRINK_VARIABLE_ORDER_FINDER_H
#define MERGE_AND_SHRINK_VARIABLE_ORDER_FINDER_H

#include <vector>

enum LinearMergeStrategyType {
    CG_GOAL_LEVEL,
    CG_GOAL_RANDOM,
    GOAL_CG_LEVEL,
    RANDOM,
    LEVEL,
    REVERSE_LEVEL
};

class VariableOrderFinder {
    const LinearMergeStrategyType linear_merge_strategy_type;
    std::vector<int> selected_vars;
    std::vector<int> remaining_vars;
    std::vector<bool> is_goal_variable;
    std::vector<bool> is_causal_predecessor;

    void select_next(int position, int var_no);
public:
    VariableOrderFinder(LinearMergeStrategyType linear_merge_strategy_type);
    ~VariableOrderFinder();
    bool done() const;
    int next();
    void dump() const;
};

#endif
