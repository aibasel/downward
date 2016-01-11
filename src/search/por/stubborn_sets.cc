#include "stubborn_sets.h"

#include "../global_operator.h"
#include "../globals.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace stubborn_sets {
struct SortFactsByVariable {
    bool operator()(const Fact &lhs, const Fact &rhs) {
        return lhs.var < rhs.var;
    }
};

StubbornSets::StubbornSets()
    : num_unpruned_successors_generated(0),
      num_pruned_successors_generated(0) {
    verify_no_axioms_no_conditional_effects();
}

/* can_disable relies on op_preconds and op_effects being sorted by
   variable */
bool StubbornSets::can_disable(int op1_no, int op2_no) {
    int i = 0;
    int j = 0;
    int num_op1_effects = op_effects[op1_no].size();
    int num_op2_preconditions = op_preconds[op2_no].size();
    while (i < num_op2_preconditions && j < num_op1_effects) {
        int read_var = op_preconds[op2_no][i].var;
        int write_var = op_effects[op1_no][j].var;
        if (read_var < write_var) {
            i++;
        } else if (read_var > write_var) {
            j++;
        } else {
            int read_value = op_preconds[op2_no][i].val;
            int write_value = op_effects[op1_no][j].val;
            if (read_value != write_value)
                return true;
            i++;
            j++;
        }
    }
    return false;
}

// can_conflict relies on op_effect being sorted by variable
bool StubbornSets::can_conflict(int op1_no, int op2_no) {
    int i = 0;
    int j = 0;
    int num_op1_effects = op_effects[op1_no].size();
    int num_op2_effects = op_effects[op2_no].size();
    while (i < num_op1_effects && j < num_op2_effects) {
        int var1 = op_effects[op1_no][i].var;
        int var2 = op_effects[op2_no][j].var;
        if (var1 < var2) {
            i++;
        } else if (var1 > var2) {
            j++;
        } else {
            int value1 = op_effects[op1_no][i].val;
            int value2 = op_effects[op2_no][j].val;
            if (value1 != value2)
                return true;
            i++;
            j++;
        }
    }
    return false;
}


void StubbornSets::compute_sorted_operators() {
    assert(op_preconds.empty());
    assert(op_effects.empty());

    for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
        GlobalOperator *op = &g_operators[op_no];

        const vector<GlobalCondition> &preconds = op->get_preconditions();
        const vector<GlobalEffect> &effects = op->get_effects();

        vector<Fact> pre;
        vector<Fact> eff;

        for (const GlobalCondition &precond : preconds) {
            int var = precond.var;
            int val = precond.val;
            Fact p(var, val);
            pre.push_back(p);
        }

        for (const GlobalEffect &effect: effects) {
            int var = effect.var;
            int val = effect.val;
            Fact e(var, val);
            eff.push_back(e);
        }

        sort(pre.begin(), pre.end(), SortFactsByVariable());
        sort(eff.begin(), eff.end(), SortFactsByVariable());

        op_preconds.push_back(pre);
        op_effects.push_back(eff);
    }
}

void StubbornSets::compute_achievers() {
    size_t num_variables = g_variable_domain.size();
    achievers.resize(num_variables);
    for (uint var_no = 0; var_no < num_variables; ++var_no) {
        achievers[var_no].resize(g_variable_domain[var_no]);
    }

    for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
        const GlobalOperator &op = g_operators[op_no];
        const vector<GlobalEffect> &effects = op.get_effects();
        for (size_t i = 0; i < effects.size(); ++i) {
            int var = effects[i].var;
            int value = effects[i].val;
            achievers[var][value].push_back(op_no);
        }
    }
}

void StubbornSets::prune_operators(
    const GlobalState &state, vector<const GlobalOperator *> &ops) {
    num_unpruned_successors_generated += ops.size();
    compute_stubborn_set(state, ops);
    num_pruned_successors_generated += ops.size();
}

void StubbornSets::print_statistics() const {
    cout << "total successors before partial-order reduction: "
         << num_unpruned_successors_generated << endl
         << "total successors after partial-order reduction: "
         << num_pruned_successors_generated << endl;
}
}
