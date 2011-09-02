#ifndef MERGE_AND_SHRINK_VARIABLE_ORDER_FINDER_H
#define MERGE_AND_SHRINK_VARIABLE_ORDER_FINDER_H

#include "merge_and_shrink_heuristic.h" // needed for MergeStrategy type;
// TODO: move that type somewhere else?

#include <vector>

class VariableOrderFinder {
    const MergeStrategy merge_strategy;
    std::vector<int> selected_vars;
    std::vector<int> remaining_vars;
    std::vector<bool> is_goal_variable;
    std::vector<bool> is_causal_predecessor;

    void select_next(int position, int var_no);
public:
    VariableOrderFinder(MergeStrategy merge_strategy, bool is_first = true);
    ~VariableOrderFinder();
    bool done() const;
    int next();
};

#endif
