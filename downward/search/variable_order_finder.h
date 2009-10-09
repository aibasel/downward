#ifndef VARIABLE_ORDER_FINDER_H
#define VARIABLE_ORDER_FINDER_H

#include <vector>

class VariableOrderFinder {
 protected:
    std::vector<int> selected_vars;
    std::vector<int> remaining_vars;
    std::vector<bool> is_goal_variable;
    std::vector<bool> is_causal_predecessor;

    void select_next(int position, int var_no);
 public:
    VariableOrderFinder(bool is_first = true);
    bool done() const;
    int next();
};

#endif
