#include "util.h"

#include "landmark_graph.h"

#include "../task_proxy.h"

#include <limits>

using namespace std;

namespace landmarks {
bool _possibly_fires(const EffectConditionsProxy &conditions, const vector<vector<int>> &lvl_var) {
    for (FactProxy cond : conditions)
        if (lvl_var[cond.get_variable().get_id()][cond.get_value()] == numeric_limits<int>::max())
            return false;
    return true;
}

unordered_map<int, int> _intersect(const unordered_map<int, int> &a, const unordered_map<int, int> &b) {
    if (a.size() > b.size())
        return _intersect(b, a);
    unordered_map<int, int> result;
    for (const auto &pair_a : a) {
        const auto it_b = b.find(pair_a.first);
        if (it_b != b.end() && it_b->second == pair_a.second)
            result.insert(pair_a);
    }
    return result;
}

bool _possibly_reaches_lm(const OperatorProxy &op, const vector<vector<int>> &lvl_var, const LandmarkNode *lmp) {
    /* Check whether operator o can possibly make landmark lmp true in a
       relaxed task (as given by the reachability information in lvl_var) */

    assert(!lvl_var.empty());

    // Test whether all preconditions of o can be reached
    // Otherwise, operator is not applicable
    PreconditionsProxy preconditions = op.get_preconditions();
    for (FactProxy pre : preconditions)
        if (lvl_var[pre.get_variable().get_id()][pre.get_value()] ==
            numeric_limits<int>::max())
            return false;

    // Go through all effects of o and check whether one can reach a
    // proposition in lmp
    for (EffectProxy effect: op.get_effects()) {
        FactProxy effect_fact = effect.get_fact();
        assert(!lvl_var[effect_fact.get_variable().get_id()].empty());
        for (const FactPair &fact : lmp->facts) {
            if (effect_fact.get_pair() == fact) {
                if (_possibly_fires(effect.get_conditions(), lvl_var))
                    return true;
                break;
            }
        }
    }

    return false;
}

OperatorProxy get_operator_or_axiom(const TaskProxy &task_proxy, int op_or_axiom_id) {
    if (op_or_axiom_id < 0) {
        return task_proxy.get_axioms()[-op_or_axiom_id - 1];
    } else {
        return task_proxy.get_operators()[op_or_axiom_id];
    }
}

int get_operator_or_axiom_id(const OperatorProxy &op) {
    if (op.is_axiom()) {
        return -op.get_id() - 1;
    } else {
        return op.get_id();
    }
}
}
