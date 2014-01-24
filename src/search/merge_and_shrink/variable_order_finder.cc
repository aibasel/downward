#include "variable_order_finder.h"

#include "../globals.h"
#include "../legacy_causal_graph.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace std;


VariableOrderFinder::VariableOrderFinder(
    LinearMergeStrategyType linear_merge_strategy_type_)
    : linear_merge_strategy_type(linear_merge_strategy_type_) {
    int var_count = g_variable_domain.size();
    if (linear_merge_strategy_type_ == MERGE_LINEAR_REVERSE_LEVEL) {
        for (int i = 0; i < var_count; ++i)
            remaining_vars.push_back(i);
    } else {
        for (int i = var_count - 1; i >= 0; i--)
            remaining_vars.push_back(i);
    }

    if (linear_merge_strategy_type == MERGE_LINEAR_CG_GOAL_RANDOM ||
        linear_merge_strategy_type == MERGE_LINEAR_RANDOM)
        random_shuffle(remaining_vars.begin(), remaining_vars.end());

    is_causal_predecessor.resize(var_count, false);
    is_goal_variable.resize(var_count, false);
    for (int i = 0; i < g_goal.size(); ++i)
        is_goal_variable[g_goal[i].first] = true;
}

VariableOrderFinder::~VariableOrderFinder() {
}

void VariableOrderFinder::select_next(int position, int var_no) {
    assert(remaining_vars[position] == var_no);
    remaining_vars.erase(remaining_vars.begin() + position);
    selected_vars.push_back(var_no);
    const vector<int> &new_vars = g_legacy_causal_graph->get_predecessors(var_no);
    for (int i = 0; i < new_vars.size(); ++i)
        is_causal_predecessor[new_vars[i]] = true;
}

bool VariableOrderFinder::done() const {
    return remaining_vars.empty();
}

int VariableOrderFinder::next() {
    assert(!done());
    if (linear_merge_strategy_type == MERGE_LINEAR_CG_GOAL_LEVEL || linear_merge_strategy_type
        == MERGE_LINEAR_CG_GOAL_RANDOM) {
        // First run: Try to find a causally connected variable.
        for (int i = 0; i < remaining_vars.size(); ++i) {
            int var_no = remaining_vars[i];
            if (is_causal_predecessor[var_no]) {
                select_next(i, var_no);
                return var_no;
            }
        }
        // Second run: Try to find a goal variable.
        for (int i = 0; i < remaining_vars.size(); ++i) {
            int var_no = remaining_vars[i];
            if (is_goal_variable[var_no]) {
                select_next(i, var_no);
                return var_no;
            }
        }
    } else if (linear_merge_strategy_type == MERGE_LINEAR_GOAL_CG_LEVEL) {
        // First run: Try to find a goal variable.
        for (int i = 0; i < remaining_vars.size(); ++i) {
            int var_no = remaining_vars[i];
            if (is_goal_variable[var_no]) {
                select_next(i, var_no);
                return var_no;
            }
        }
        // Second run: Try to find a causally connected variable.
        for (int i = 0; i < remaining_vars.size(); ++i) {
            int var_no = remaining_vars[i];
            if (is_causal_predecessor[var_no]) {
                select_next(i, var_no);
                return var_no;
            }
        }
    } else if (linear_merge_strategy_type == MERGE_LINEAR_RANDOM ||
               linear_merge_strategy_type == MERGE_LINEAR_LEVEL ||
               linear_merge_strategy_type == MERGE_LINEAR_REVERSE_LEVEL) {
        int var_no = remaining_vars[0];
        select_next(0, var_no);
        return var_no;
    }
    cerr << "Relevance analysis has not been performed." << endl;
    exit_with(EXIT_INPUT_ERROR);
}

void VariableOrderFinder::dump() const {
    cout << "Linear merge strategy type:";
    switch (linear_merge_strategy_type) {
    case MERGE_LINEAR_CG_GOAL_LEVEL:
        cout << "CG/GOAL, tie breaking on level (main)";
        break;
    case MERGE_LINEAR_CG_GOAL_RANDOM:
        cout << "CG/GOAL, tie breaking random";
        break;
    case MERGE_LINEAR_GOAL_CG_LEVEL:
        cout << "GOAL/CG, tie breaking on level";
        break;
    case MERGE_LINEAR_RANDOM:
        cout << "random";
        break;
    case MERGE_LINEAR_LEVEL:
        cout << "by level";
        break;
    case MERGE_LINEAR_REVERSE_LEVEL:
        cout << "by reverse level";
        break;
    default:
        ABORT("Unknown merge strategy.");
    }
    cout << endl;
}
