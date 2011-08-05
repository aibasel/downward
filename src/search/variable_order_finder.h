#ifndef VARIABLE_ORDER_FINDER_H
#define VARIABLE_ORDER_FINDER_H

#include "mas_heuristic.h" // needed for MergeStrategy type;
// TODO: move that type somewhere else?

#include <vector>

class VariableOrderFinder {
    MergeStrategy merge_strategy;
protected:
    std::vector<int> selected_vars;
    std::vector<int> remaining_vars;
    std::vector<bool> is_goal_variable;
    std::vector<bool> is_causal_predecessor;

    void select_next(int position, int var_no);
public:
    VariableOrderFinder(MergeStrategy merge_strategy, double mix_parameter,
                        bool is_first = true);
    bool done() const;
    int next();
};

#endif
