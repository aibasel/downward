#include "stubborn_sets.h"

#include "../global_operator.h"
#include "../globals.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace StubbornSets {

StubbornSets::StubbornSets()
    : unpruned_successors_generated(0),
      pruned_successors_generated(0) {
}

StubbornSets::~StubbornSets() {
}

inline bool operator<(const Fact &lhs, const Fact &rhs) {
    return lhs.var < rhs.var;
}

bool StubbornSets::can_disable(int op1_no, int op2_no) {
    size_t i = 0;
    size_t j = 0;
    while (i < op_preconds[op2_no].size() && j < op_effects[op1_no].size()) {
        int read_var = op_preconds[op2_no][i].var;
        int write_var = op_effects[op1_no][j].var;
        if (read_var < write_var)
            i++;
        else if (read_var == write_var) {
            int read_value = op_preconds[op2_no][i].val;
            int write_value = op_effects[op1_no][j].val;
            if (read_value != write_value)
                return true;
            i++;
            j++;
        } else {
            // read_var > write_var
            j++;
        }
    }
    return false;
}

bool StubbornSets::can_conflict(int op1_no, int op2_no) {
    size_t i = 0;
    size_t j = 0;
    while (i < op_effects[op1_no].size() && j < op_effects[op2_no].size()) {
        int var1 = op_effects[op1_no][i].var;
        int var2 = op_effects[op2_no][j].var;
        if (var1 < var2)
            i++;
        else if (var1 == var2) {
            int value1 = op_effects[op1_no][i].val;
            int value2 = op_effects[op2_no][j].val;
            if (value1 != value2)
                return true;
            i++;
            j++;
        } else {
            // var1 > var2
            j++;
        }
    }
    return false;
}


bool StubbornSets::interfere(int op1_no, int op2_no) {
    return can_disable(op1_no, op2_no) || can_conflict(op1_no, op2_no) || can_disable(op2_no, op1_no);
}

void StubbornSets::compute_sorted_operators() {
    for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
        GlobalOperator *op = &g_operators[op_no];
	
	const vector<GlobalCondition> &preconds = op->get_preconditions();
	const vector<GlobalEffect> &effects = op->get_effects();

        vector<Fact> pre;
        vector<Fact> eff;

	for (size_t i = 0; i < preconds.size(); i++) {
	    int var = preconds[i].var;
	    int val = preconds[i].val;
	    Fact p(var, val);
	    pre.push_back(p);
	}
	
	for (size_t i = 0; i < effects.size(); i++) {
	    int var = effects[i].var;
	    int val = effects[i].val;
	    Fact e(var, val);
	    eff.push_back(e);
	}
	
        if (pre.size() != 0) {
            sort(pre.begin(), pre.end());
            for (size_t i = 0; i < pre.size() - 1; ++i) {
                assert(pre[i].var < pre[i + 1].var);
            }
        }
        sort(eff.begin(), eff.end());
        for (size_t i = 0; i < eff.size() - 1; ++i) {
            assert(eff[i].var < eff[i + 1].var);
        }
        op_preconds.push_back(pre);
        op_effects.push_back(eff);
    }
}

void StubbornSets::compute_achievers() {
    size_t num_variables = g_variable_domain.size();
    achievers.resize(num_variables);
    for (uint var_no = 0; var_no < num_variables; ++var_no)
        achievers[var_no].resize(g_variable_domain[var_no]);

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
    const GlobalState &state, std::vector<const GlobalOperator *> &ops) {
    unpruned_successors_generated += ops.size();
    do_pruning(state, ops);
    pruned_successors_generated += ops.size();
}

void StubbornSets::dump_statistics() const {
    cout << "total successors before partial-order reduction: "
         << unpruned_successors_generated << endl
         << "total successors after partial-order reduction: "
         << pruned_successors_generated << endl;
}

}
