#include "stubborn_sets.h"

#include "../task_tools.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace stubborn_sets {
// TODO consider moving this to a more central position or implementing it in FactProxy.
static inline Fact get_fact(FactProxy fact_proxy) {
    int var_id = fact_proxy.get_variable().get_id();
    int value = fact_proxy.get_value();
    return Fact(var_id, value);
}

struct SortFactsByVariable {
    bool operator()(const Fact &lhs, const Fact &rhs) {
        return lhs.var < rhs.var;
    }
};

// Relies on both fact sets being sorted by variable.
bool contain_conflicting_fact(const vector<Fact> &facts1,
                              const vector<Fact> &facts2) {
    auto facts1_it = facts1.begin();
    auto facts2_it = facts2.begin();
    while (facts1_it != facts1.end() && facts2_it != facts2.end()) {
        if (facts1_it->var < facts2_it->var) {
            ++facts1_it;
        } else if (facts1_it->var > facts2_it->var) {
            ++facts2_it;
        } else {
            if (facts1_it->value != facts2_it->value)
                return true;
            ++facts1_it;
            ++facts2_it;
        }
    }
    return false;
}

StubbornSets::StubbornSets()
    : num_unpruned_successors_generated(0),
      num_pruned_successors_generated(0) {
    verify_no_axioms(task_proxy);
    verify_no_conditional_effects(task_proxy);
    compute_sorted_operators();
    compute_achievers();
}

Fact StubbornSets::find_unsatisfied_goal(const State &state) {
    for (FactProxy goal : task_proxy.get_goals()) {
        int goal_var_id = goal.get_variable().get_id();
        if (state[goal_var_id] != goal)
            return get_fact(goal);
    }
    return Fact(-1, -1);
}

Fact StubbornSets::find_unsatisfied_precondition(OperatorProxy op,
                                                 const State &state) {
    for (FactProxy precondition : op.get_preconditions()) {
        int var_id = precondition.get_variable().get_id();
        if (state[var_id] != precondition)
            return get_fact(precondition);
    }
    return Fact(-1, -1);
}

// Relies on op_preconds and op_effects being sorted by variable.
bool StubbornSets::can_disable(OperatorProxy op1, OperatorProxy op2) {
    return contain_conflicting_fact(sorted_op_effects[op1.get_id()],
                                    sorted_op_preconditions[op2.get_id()]);
}

// Relies on op_effect being sorted by variable.
bool StubbornSets::can_conflict(OperatorProxy op1, OperatorProxy op2) {
    return contain_conflicting_fact(sorted_op_effects[op1.get_id()],
                                    sorted_op_effects[op2.get_id()]);
}

void StubbornSets::compute_sorted_operators() {
    assert(sorted_op_preconditions.empty());
    assert(sorted_op_effects.empty());

    for (const OperatorProxy op : task_proxy.get_operators()) {
        vector<Fact> preconditions;
        for (const FactProxy pre : op.get_preconditions()) {
            preconditions.push_back(get_fact(pre));
        }
        sort(preconditions.begin(), preconditions.end(), SortFactsByVariable());
        sorted_op_preconditions.push_back(preconditions);

        vector<Fact> effects;
        for (const EffectProxy eff : op.get_effects()) {
            effects.push_back(get_fact(eff.get_fact()));
        }
        sort(effects.begin(), effects.end(), SortFactsByVariable());
        sorted_op_effects.push_back(effects);
    }
}

void StubbornSets::compute_achievers() {
    VariablesProxy vars = task_proxy.get_variables();
    achievers.reserve(vars.size());
    for (const VariableProxy var : vars) {
        achievers.push_back(vector<vector<int>>(var.get_domain_size()));
    }

    for (const OperatorProxy op : task_proxy.get_operators()) {
        for (const EffectProxy effect : op.get_effects()) {
            FactProxy fact = effect.get_fact();
            int var_id = fact.get_variable().get_id();
            int value = fact.get_value();
            achievers[var_id][value].push_back(op.get_id());
        }
    }
}

bool StubbornSets::mark_as_stubborn(int op_no) {
    if (!stubborn[op_no]) {
        stubborn[op_no] = true;
        stubborn_queue.push_back(op_no);
        return true;
    }
    return false;
}

void StubbornSets::prune_operators(
    const State &state, vector<OperatorProxy> &ops) {
    num_unpruned_successors_generated += ops.size();

    // Clear stubborn set from previous call.
    stubborn.clear();
    stubborn.assign(task_proxy.get_operators().size(), false);
    assert(stubborn_queue.empty());

    initialize_stubborn_set(state);
    /* Iteratively insert operators to stubborn according to the
       definition of strong stubborn sets until a fixpoint is reached. */
    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();
        stubborn_queue.pop_back();
        OperatorProxy op = task_proxy.get_operators()[op_no];
        handle_stubborn_operator(state, op);
    }

    // Now check which applicable operators are in the stubborn set.
    vector<OperatorProxy> remaining_ops;
    remaining_ops.reserve(ops.size());
    for (OperatorProxy op : ops) {
        if (stubborn[op.get_id()])
            remaining_ops.push_back(op);
    }
    if (remaining_ops.size() != ops.size()) {
        ops.swap(remaining_ops);
    }

    num_pruned_successors_generated += ops.size();
}

void StubbornSets::print_statistics() const {
    cout << "total successors before partial-order reduction: "
         << num_unpruned_successors_generated << endl
         << "total successors after partial-order reduction: "
         << num_pruned_successors_generated << endl;
}
}
