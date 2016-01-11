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

// Relies on both fact sets being sorted by variable.
bool contain_conflicting_fact(const std::vector<Fact> &fact_set_1,
                              const std::vector<Fact> &fact_set_2) {
    auto f1 = fact_set_1.begin();
    auto f2 = fact_set_2.begin();
    while (f1 != fact_set_1.end() && f2 != fact_set_2.end()) {
        if (f1->var < f2->var) {
            ++f1;
        } else if (f1->var > f2->var) {
            ++f2;
        } else {
            if (f1->val != f2->val)
                return true;
            ++f1;
            ++f2;
        }
    }
    return false;
}

StubbornSets::StubbornSets()
    : num_unpruned_successors_generated(0),
      num_pruned_successors_generated(0) {
    verify_no_axioms_no_conditional_effects();
}

// Relies on op_preconds and op_effects being sorted by variable.
bool StubbornSets::can_disable(int op1_no, int op2_no) {
    return contain_conflicting_fact(op_effects[op1_no], op_preconds[op2_no]);
}

// Relies on op_effect being sorted by variable.
bool StubbornSets::can_conflict(int op1_no, int op2_no) {
    return contain_conflicting_fact(op_effects[op1_no], op_effects[op2_no]);
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
