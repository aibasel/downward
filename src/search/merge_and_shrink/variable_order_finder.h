#ifndef MERGE_AND_SHRINK_VARIABLE_ORDER_FINDER_H
#define MERGE_AND_SHRINK_VARIABLE_ORDER_FINDER_H

#include <vector>

enum LinearMergeStrategyType {
    MERGE_LINEAR_CG_GOAL_LEVEL,
    MERGE_LINEAR_CG_GOAL_RANDOM,
    MERGE_LINEAR_GOAL_CG_LEVEL,
    MERGE_LINEAR_RANDOM,
    MERGE_LINEAR_LEVEL,
    MERGE_LINEAR_REVERSE_LEVEL
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
