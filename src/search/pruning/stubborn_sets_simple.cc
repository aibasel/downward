#include "stubborn_sets_simple.h"

#include "../globals.h"
#include "../global_operator.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace stubborn_sets_simple {
/* Implementation of simple instantiation of strong stubborn sets.
   Disjunctive action landmarks are computed trivially.*/

// Return the first unsatified goal pair, or (-1, -1) if there is none.
static inline Fact find_unsatisfied_goal(const GlobalState &state) {
    for (const pair<int, int> &goal : g_goal) {
        int goal_var = goal.first;
        int goal_value = goal.second;
        if (state[goal_var] != goal_value)
            return Fact(goal_var, goal_value);
    }
    return Fact(-1, -1);
}

// Return the first unsatified precondition, or (-1, -1) if there is none.
static inline Fact find_unsatisfied_precondition(
    const GlobalOperator &op, const GlobalState &state) {
    for (const GlobalCondition &precondition : op.get_preconditions()) {
        int var = precondition.var;
        int value = precondition.val;
        if (state[var] != value)
            return Fact(var, value);
    }
    return Fact(-1, -1);
}

StubbornSetsSimple::StubbornSetsSimple() {
    compute_interference_relation();
    cout << "pruning method: stubborn sets simple" << endl;
}

void StubbornSetsSimple::compute_interference_relation() {
    int num_operators = g_operators.size();
    interference_relation.resize(num_operators);

    /*
       TODO: as interference is symmetric, we only need to compute the
       relation for operators (o1, o2) with (o1 < o2) and add a lookup
       method that looks up (i, j) if i < j and (j, i) otherwise.
    */
    for (int op1_no = 0; op1_no < num_operators; ++op1_no) {
        vector<int> &interfere_op1 = interference_relation[op1_no];
        for (int op2_no = 0; op2_no < num_operators; ++op2_no) {
            if (op1_no != op2_no && interfere(op1_no, op2_no)) {
                interfere_op1.push_back(op2_no);
            }
        }
    }
}

// Add all operators that achieve the fact (var, value) to stubborn set.
void StubbornSetsSimple::add_necessary_enabling_set(Fact fact) {
    for (int op_no : achievers[fact.var][fact.value]) {
        mark_as_stubborn(op_no);
    }
}

// Add all operators that interfere with op.
void StubbornSetsSimple::add_interfering(int op_no) {
    for (int interferer_no : interference_relation[op_no]) {
        mark_as_stubborn(interferer_no);
    }
}

void StubbornSetsSimple::initialize_stubborn_set(const GlobalState &state) {
    // Add a necessary enabling set for an unsatisfied goal.
    Fact unsatisfied_goal = find_unsatisfied_goal(state);
    assert(unsatisfied_goal.var != -1);
    add_necessary_enabling_set(unsatisfied_goal);
}

void StubbornSetsSimple::handle_stubborn_operator(const GlobalState &state,
                                                  int op_no) {
    const GlobalOperator &op = g_operators[op_no];
    Fact unsatisfied_precondition = find_unsatisfied_precondition(op, state);
    if (unsatisfied_precondition.var == -1) {
        /* no unsatisfied precondition found
           => operator is applicable
           => add all interfering operators */
        add_interfering(op_no);
    } else {
        /* unsatisfied precondition found
           => add a necessary enabling set for it */
        add_necessary_enabling_set(unsatisfied_precondition);
    }
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Stubborn sets simple",
        "stubborn sets with simple instantiations of design choices");

    if (parser.dry_run()) {
	return nullptr;
    }

    return make_shared<StubbornSetsSimple>();
}

static PluginShared<PruningMethod> _plugin("stubborn_sets_simple", _parse);
}
