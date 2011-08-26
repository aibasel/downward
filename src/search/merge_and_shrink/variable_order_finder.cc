#include "variable_order_finder.h"

#include "merge_and_shrink_heuristic.h" // needed for MergeStrategy type;
// TODO: move that type somewhere else?

#include "../causal_graph.h"
#include "../globals.h"

#include <cassert>
#include <cstdlib>
#include <vector>
using namespace std;


VariableOrderFinder::VariableOrderFinder(
    MergeStrategy merge_strategy_, bool is_first)
    : merge_strategy(merge_strategy_) {
    int var_count = g_variable_domain.size();
    if (merge_strategy_ == MERGE_LINEAR_REVERSE_LEVEL) {
        for (int i = 0; i < var_count; ++i)
            remaining_vars.push_back(i);
    } else {
        for (int i = var_count - 1; i >= 0; i--)
            remaining_vars.push_back(i);
    }

    if (merge_strategy == MERGE_LINEAR_CG_GOAL_RANDOM ||
        merge_strategy == MERGE_LINEAR_RANDOM ||
        !is_first)
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
    const vector<int> &new_vars = g_causal_graph->get_predecessors(var_no);
    for (int i = 0; i < new_vars.size(); ++i)
        is_causal_predecessor[new_vars[i]] = true;
}

bool VariableOrderFinder::done() const {
    return remaining_vars.empty();
}

int VariableOrderFinder::next() {
    assert(!done());
    if (merge_strategy == MERGE_LINEAR_CG_GOAL_LEVEL || merge_strategy
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
    } else if (merge_strategy == MERGE_LINEAR_GOAL_CG_LEVEL) {
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
    } else if (merge_strategy == MERGE_LINEAR_RANDOM ||
               merge_strategy == MERGE_LINEAR_LEVEL ||
               merge_strategy == MERGE_LINEAR_REVERSE_LEVEL) {
        int var_no = remaining_vars[0];
        select_next(0, var_no);
        return var_no;
    } else if (merge_strategy == MERGE_DFP) {
        /* TODO: Implement this, but not here, as it is *not* a linear
           merge strategy. */
        abort();
    }

    // This should never happen, at least if we did relevance analysis.
    abort();
}
