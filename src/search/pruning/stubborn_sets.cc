#include "stubborn_sets.h"

#include "../task_tools.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace stubborn_sets {
// Relies on both fact sets being sorted by variable.
bool contain_conflicting_fact(const vector<FactPair> &facts1,
                              const vector<FactPair> &facts2) {
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

void StubbornSets::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    TaskProxy task_proxy(*task);
    verify_no_axioms(task_proxy);
    verify_no_conditional_effects(task_proxy);
    compute_sorted_operators(task_proxy);
    GoalsProxy task_goals = task_proxy.get_goals();
    goals.reserve(task_goals.size());
    for (FactProxy goal : task_goals) {
        goals.push_back(goal.get_pair());
    }
    compute_achievers(task_proxy);
    num_unpruned_successors_generated = 0;
    num_pruned_successors_generated = 0;
}

// Relies on op_preconds and op_effects being sorted by variable.
bool StubbornSets::can_disable(int op1_no, int op2_no) {
    return contain_conflicting_fact(sorted_op_effects[op1_no],
                                    sorted_op_preconditions[op2_no]);
}

// Relies on op_effect being sorted by variable.
bool StubbornSets::can_conflict(int op1_no, int op2_no) {
    return contain_conflicting_fact(sorted_op_effects[op1_no],
                                    sorted_op_effects[op2_no]);
}

void StubbornSets::compute_sorted_operators(const TaskProxy &task_proxy) {
    assert(sorted_op_preconditions.empty());
    assert(sorted_op_effects.empty());

    for (const OperatorProxy op : task_proxy.get_operators()) {
        PreconditionsProxy preconditions = op.get_preconditions();
        vector<FactPair> sorted_preconditions;
        sorted_preconditions.reserve(preconditions.size());
        for (const FactProxy pre : preconditions) {
            sorted_preconditions.push_back(pre.get_pair());
        }
        sort(sorted_preconditions.begin(), sorted_preconditions.end());
        sorted_op_preconditions.push_back(sorted_preconditions);

        EffectsProxy effects = op.get_effects();
        vector<FactPair> sorted_effects;
        sorted_effects.reserve(effects.size());
        for (const EffectProxy eff : effects) {
            sorted_effects.push_back(eff.get_fact().get_pair());
        }
        sort(sorted_effects.begin(), sorted_effects.end());
        sorted_op_effects.push_back(sorted_effects);
    }
}

void StubbornSets::compute_achievers(const TaskProxy &task_proxy) {
    VariablesProxy vars = task_proxy.get_variables();
    achievers.reserve(vars.size());
    for (const VariableProxy var : vars) {
        achievers.push_back(vector<vector<int>>(var.get_domain_size()));
    }

    for (const OperatorProxy op : task_proxy.get_operators()) {
        for (const EffectProxy effect : op.get_effects()) {
            FactPair fact = effect.get_fact().get_pair();
            achievers[fact.var][fact.value].push_back(op.get_id());
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
    const State &state, vector<int> &op_ids) {
    num_unpruned_successors_generated += op_ids.size();

    // Clear stubborn set from previous call.
    stubborn.clear();
    int num_operators = sorted_op_effects.size();
    stubborn.assign(num_operators, false);
    assert(stubborn_queue.empty());

    initialize_stubborn_set(state);
    /* Iteratively insert operators to stubborn according to the
       definition of strong stubborn sets until a fixpoint is reached. */
    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();
        stubborn_queue.pop_back();
        handle_stubborn_operator(state, op_no);
    }

    // Now check which applicable operators are in the stubborn set.
    vector<int> remaining_op_ids;
    remaining_op_ids.reserve(op_ids.size());
    for (int op_id : op_ids) {
        if (stubborn[op_id])
            remaining_op_ids.push_back(op_id);
    }
    if (remaining_op_ids.size() != op_ids.size()) {
        op_ids.swap(remaining_op_ids);
    }

    num_pruned_successors_generated += op_ids.size();
}

void StubbornSets::print_statistics() const {
    cout << "total successors before partial-order reduction: "
         << num_unpruned_successors_generated << endl
         << "total successors after partial-order reduction: "
         << num_pruned_successors_generated << endl;
}
}
