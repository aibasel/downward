#include "stubborn_sets_simple.h"

#include "../globals.h"
#include "../global_operator.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace StubbornSetsSimple {
/* Implementation of simple instantiation of strong stubborn sets.
   Disjunctive action landmarks are computed trivially.*/

/* TODO: get_op_index belongs to a central place.
   We currently have copies of it in different parts of the code. */
static inline int get_op_index(const GlobalOperator *op) {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && (uint)op_index < g_operators.size());
    return op_index;
}

/* Return the first unsatified goal pair, or (-1, -1) if there is none.
   TODO: use Fact instead of pair<int, int> */
static inline pair<int, int> find_unsatisfied_goal(const GlobalState &state) {
    for (const pair<int, int>& goal : g_goal) {
        int goal_var = goal.first;
        int goal_value = goal.second;
        if (state[goal_var] != goal_value)
            return make_pair(goal_var, goal_value);
    }
    return make_pair(-1, -1);
}

/* Return the first unsatified precondition, or (-1, -1) if there is none.
   TODO: use Fact instead of pair<int, int> */
static inline pair<int, int> find_unsatisfied_precondition(
    const GlobalOperator &op, const GlobalState &state) {
    const vector<GlobalCondition> &preconds = op.get_preconditions();
    for (const GlobalCondition &precond : preconds) {
        int var = precond.var;
        int value = precond.val;
        if (state[var] != value)
            return make_pair(var, value);
    }

    return make_pair(-1, -1);
}

void StubbornSetsSimple::initialize() {
    verify_no_axioms_no_conditional_effects();
    compute_interference_relation();
    compute_achievers();
    cout << "partial order reduction method: stubborn sets simple" << endl;
}

void StubbornSetsSimple::compute_interference_relation() {
    compute_sorted_operators();

    uint num_operators = g_operators.size();
    interference_relation.resize(num_operators);
    
    /* 
       TODO: as interference is symmetric, we only need to compute the
       relation for operators (o1, o2) with (o1 < o2) and add a lookup
       method that looks up (i, j) if i < j and (j, i) otherwise.
    */
    for (size_t op1_no = 0; op1_no < num_operators; ++op1_no) {
        vector<int> &interfere_op1 = interference_relation[op1_no];
        for (size_t op2_no = 0; op2_no < num_operators; ++op2_no) {
            if (op1_no != op2_no && interfere(op1_no, op2_no)) {
                interfere_op1.push_back(op2_no);
	    }
        }
    }
}

void StubbornSetsSimple::mark_as_stubborn(int op_no) {
    if (!stubborn[op_no]) {
        stubborn[op_no] = true;
        stubborn_queue.push_back(op_no);
    }
}

/* Add all operators that achieve the fact (var, value) to stubborn set.
   TODO: use Fact instead of pair<int, int> */
void StubbornSetsSimple::add_necessary_enabling_set(pair<int, int> fact) {
    int var = fact.first;
    int value = fact.second;
    for (int op_no : achievers[var][value]) {
        mark_as_stubborn(op_no);
    }
}

// Add all operators that interfere with op.
void StubbornSetsSimple::add_interfering(int op_no) {
    for (int interferer_no : interference_relation[op_no]) {
	mark_as_stubborn(interferer_no);
    }
}

void StubbornSetsSimple::do_pruning(
    const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) {
    // Clear stubborn set from previous call.
    stubborn.clear();
    stubborn.assign(g_operators.size(), false);

    // Add a necessary enabling set for an unsatisfied goal.
    pair<int, int> goal_pair = find_unsatisfied_goal(state);
    if (goal_pair.first == -1) {
	// goal state encountered
	applicable_ops.clear();
	return;
    }
    
    add_necessary_enabling_set(goal_pair);

    /* Iteratively insert operators to stubborn according to the
       definition of strong stubborn sets until a fixpoint is reached. */
    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();
        stubborn_queue.pop_back();
        const GlobalOperator &op = g_operators[op_no];
        pair<int, int> fact = find_unsatisfied_precondition(op, state);
        if (fact.first == -1) {
            /* no unsatisfied precondition found
	       => operator is applicable
	       => add all interfering operators */
            add_interfering(op_no);
        } else {
            /* unsatisfied precondition found
	       => add a necessary enabling set for it */
            add_necessary_enabling_set(fact);
        }
    }

    // Now check which applicable operators are in the stubborn set.
    vector<const GlobalOperator *> pruned_ops;
    pruned_ops.reserve(applicable_ops.size());
    for (const GlobalOperator *op : applicable_ops) {
        int op_no = get_op_index(op);
        if (stubborn[op_no])
            pruned_ops.push_back(op);
    }
    if (pruned_ops.size() != applicable_ops.size()) {
        applicable_ops.swap(pruned_ops);
        sort(applicable_ops.begin(), applicable_ops.end());
    }
}

static shared_ptr<PORMethod> _parse(OptionParser &parser) {
    parser.document_synopsis("Stubborn sets simple", 
			     "stubborn sets with simple instantiations of design choices");

    return make_shared<StubbornSetsSimple>();
}

static PluginShared<PORMethod> _plugin("stubborn_sets_simple", _parse);
}
