#include "stubborn_sets_simple.h"

#include "../globals.h"
#include "../global_operator.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace StubbornSetsSimple {
// Implementation of simple instantiation of strong stubborn sets.
// Disjunctive action landmarks are computed trivially.
//
//
// TODO: get_op_index belongs to a central place.
// We currently have copies of it in different parts of the code.
//
//
// Internal representation of operator preconditions.
// Only used during computation of interference relation.
// op_preconds[i] contains precondition facts of the i-th
// operator.
//
//
// Internal representation of operator effects.
// Only used during computation of interference relation.
// op_effects[i] contains effect facts of the i-th
// operator.

static inline int get_op_index(const GlobalOperator *op) {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < g_operators.size());
    return op_index;
}

// Return the first unsatified goal pair, or (-1, -1) if there is none.
static inline pair<int, int> find_unsatisfied_goal(const GlobalState &state) {
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int goal_var = g_goal[i].first;
        int goal_value = g_goal[i].second;
        if (state[goal_var] != goal_value)
            return make_pair(goal_var, goal_value);
    }
    return make_pair(-1, -1);
}


// Return the first unsatified precondition, or (-1, -1) if there is none.
static inline pair<int, int> find_unsatisfied_precondition(
    const GlobalOperator &op, const GlobalState &state) {
    const vector<GlobalCondition> &preconds = op.get_preconditions();
    for (size_t i = 0; i < preconds.size(); ++i) {
        int var = preconds[i].var;
        int value = preconds[i].val;
        if (state[var] != value)
            return make_pair(var, value);
    }

    return make_pair(-1, -1);
}


StubbornSetsSimple::StubbornSetsSimple() {
    verify_no_axioms_no_conditional_effects();
    compute_interference_relation();
    compute_achievers();
}

StubbornSetsSimple::~StubbornSetsSimple() {
}

void StubbornSetsSimple::dump_options() const {
    cout << "partial order reduction method: simple stubborn sets" << endl;
}

void StubbornSetsSimple::compute_interference_relation() {
    compute_sorted_operators();

    int num_interfering_pairs = 0;

    size_t num_operators = g_operators.size();
    interference_relation.resize(num_operators);
    for (size_t op1_no = 0; op1_no < num_operators; ++op1_no) {
        vector<int> &interfere_op1 = interference_relation[op1_no];
        for (size_t op2_no = 0; op2_no < num_operators; ++op2_no) {
            if (op1_no != op2_no && interfere(op1_no, op2_no))
                interfere_op1.push_back(op2_no);
        }

	num_interfering_pairs += interfere_op1.size();
    }
    
    cout << "Number of interfering operator pairs: " << num_interfering_pairs << endl;
    
}

void StubbornSetsSimple::mark_as_stubborn(int op_no) {
    if (!stubborn[op_no]) {
        stubborn[op_no] = true;
        stubborn_queue.push_back(op_no);
    }
}

// Add all operators that achieve the fact (var, value) to stubborn set.
void StubbornSetsSimple::add_nes_for_fact(pair<int, int> fact) {
    int var = fact.first;
    int value = fact.second;
    const vector<int> &op_nos = achievers[var][value];
    for (size_t i = 0; i < op_nos.size(); ++i)
        mark_as_stubborn(op_nos[i]);
}

// Add all operators that interfere with op.
void StubbornSetsSimple::add_interfering(int op_no) {
    const vector<int> &interferers = interference_relation[op_no];
    for (size_t i = 0; i < interferers.size(); ++i)
        mark_as_stubborn(interferers[i]);
}

void StubbornSetsSimple::do_pruning(
    const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) {
    // Clear stubborn set from previous call.
    stubborn.clear();
    stubborn.assign(g_operators.size(), false);

    // Add a necessary enabling set for an unsatisfied goal.
    pair<int, int> goal_pair = find_unsatisfied_goal(state);
    assert(goal_pair.first != -1);
    add_nes_for_fact(goal_pair);

    // Iteratively insert operators to stubborn according to the
    // definition of strong stubborn sets until a fixpoint is reached.
    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();
        stubborn_queue.pop_back();
        const GlobalOperator &op = g_operators[op_no];
        pair<int, int> fact = find_unsatisfied_precondition(op, state);
        if (fact.first == -1) {
            // no unsatisfied precondition found
            // => operator is applicable
            // => add all interfering operators
            add_interfering(op_no);
        } else {
            // unsatisfied precondition found
            // => add an enabling set for it
            add_nes_for_fact(fact);
        }
    }

    // Now check which applicable operators are in the stubborn set.
    vector<const GlobalOperator *> pruned_ops;
    pruned_ops.reserve(applicable_ops.size());
    for (size_t i = 0; i < applicable_ops.size(); ++i) {
        const GlobalOperator *op = applicable_ops[i];
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
    parser.document_synopsis("Stubborn sets simple", "stubborn sets with simple instantiations of design choices");

    return make_shared<StubbornSetsSimple>();
}

static PluginShared<PORMethod> _plugin("stubborn_sets_simple", _parse);
}
