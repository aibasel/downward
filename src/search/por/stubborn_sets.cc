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
            if (facts1_it->val != facts2_it->val)
                return true;
            ++facts1_it;
            ++facts2_it;
        }
    }
    return false;
}

template<typename T>
vector<Fact> get_sorted_fact_set(const vector<T> &facts) {
    vector<Fact> result;
    for (const T &fact : facts) {
        result.emplace_back(fact.var, fact.val);
    }
    sort(result.begin(), result.end(), SortFactsByVariable());
    return result;
}

StubbornSets::StubbornSets()
    : num_unpruned_successors_generated(0),
      num_pruned_successors_generated(0) {
    verify_no_axioms_no_conditional_effects();
}

// Relies on op_preconds and op_effects being sorted by variable.
bool StubbornSets::can_disable(int op1_no, int op2_no) {
    return contain_conflicting_fact(op_effects[op1_no], op_preconditions[op2_no]);
}

// Relies on op_effect being sorted by variable.
bool StubbornSets::can_conflict(int op1_no, int op2_no) {
    return contain_conflicting_fact(op_effects[op1_no], op_effects[op2_no]);
}

void StubbornSets::compute_sorted_operators() {
    assert(op_preconditions.empty());
    assert(op_effects.empty());

    for (const GlobalOperator &op : g_operators) {
        op_preconditions.push_back(get_sorted_fact_set(op.get_preconditions()));
        op_effects.push_back(get_sorted_fact_set(op.get_effects()));
    }
}

void StubbornSets::compute_achievers() {
    achievers.reserve(g_variable_domain.size());
    for (int domain_size : g_variable_domain) {
        achievers.push_back(vector<vector<int>>(domain_size));
    }

    for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
        const GlobalOperator &op = g_operators[op_no];
        for (const GlobalEffect &effect : op.get_effects()) {
            achievers[effect.var][effect.val].push_back(op_no);
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
