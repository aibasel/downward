#include "util.h"

#include "landmark_graph.h"
#include "../global_operator.h"

#include <limits>

using namespace std;

bool _possibly_fires(const vector<GlobalCondition> &conditions, const vector<vector<int> > &lvl_var) {
    for (size_t i = 0; i < conditions.size(); ++i)
        if (lvl_var[conditions[i].var][conditions[i].val] ==
            numeric_limits<int>::max())
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

bool _possibly_reaches_lm(const GlobalOperator &o, const vector<vector<int> > &lvl_var, const LandmarkNode *lmp) {
    /* Check whether operator o can possibly make landmark lmp true in a
       relaxed task (as given by the reachability information in lvl_var) */

    assert(!lvl_var.empty());

    // Test whether all preconditions of o can be reached
    // Otherwise, operator is not applicable
    const vector<GlobalCondition> &preconditions = o.get_preconditions();
    for (size_t i = 0; i < preconditions.size(); ++i)
        if (lvl_var[preconditions[i].var][preconditions[i].val] ==
            numeric_limits<int>::max())
            return false;

    // Go through all effects of o and check whether one can reach a
    // proposition in lmp
    const vector<GlobalEffect> &effects = o.get_effects();
    for (size_t i = 0; i < effects.size(); ++i) {
        assert(!lvl_var[effects[i].var].empty());
        for (size_t j = 0; j < lmp->vars.size(); ++j) {
            if (effects[i].var == lmp->vars[j] && effects[i].val == lmp->vals[j]) {
                if (_possibly_fires(effects[i].conditions, lvl_var))
                    return true;
                break;
            }
        }
    }

    return false;
}
