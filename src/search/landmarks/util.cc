#include "util.h"

#include "landmark_graph.h"

#include "../operator.h"

#include <limits>

using namespace std;
using namespace __gnu_cxx;

bool _possibly_fires(const vector<Condition> &conditions, const vector<vector<int> > &lvl_var) {
    for (int j = 0; j < conditions.size(); j++)
        if (lvl_var[conditions[j].var][conditions[j].val] ==
            numeric_limits<int>::max())
            return false;
    return true;
}


hash_map<int, int> _intersect(const hash_map<int, int> &a, const hash_map<int, int> &b) {
    if (a.size() > b.size())
        return _intersect(b, a);
    hash_map<int, int> result;
    for (hash_map<int, int>::const_iterator it1 = a.begin(); it1 != a.end(); it1++) {
        hash_map<int, int>::const_iterator it2 = b.find(it1->first);
        if (it2 != b.end() && it2->second == it1->second)
            result.insert(*it1);
    }
    return result;
}


bool _possibly_reaches_lm(const Operator &o, const vector<vector<int> > &lvl_var, const LandmarkNode *lmp) {
    /* Check whether operator o can possibly make landmark lmp true in a
       relaxed task (as given by the reachability information in lvl_var) */

    assert(!lvl_var.empty());

    // Test whether all preconditions of o can be reached
    // Otherwise, operator is not applicable
    const vector<Condition> &preconditions = o.get_preconditions();
    for (unsigned i = 0; i < preconditions.size(); i++)
        if (lvl_var[preconditions[i].var][preconditions[i].val] ==
            numeric_limits<int>::max())
            return false;

    // Test effect conditions
    bool reaches_lm = false;
    // Go through all effects of o...
    const vector<Effect> &effects = o.get_effects();
    for (unsigned i = 0; i < effects.size(); i++) {
        // If lmp is a conditional effect, check the condition
        bool effect_condition_reachable = false;
        assert(!lvl_var[effects[i].var].empty());
        bool correct_effect = false;
        // Check whether *this* effect of o reaches a proposition in lmp...
        for (unsigned int j = 0; j < lmp->vars.size(); j++) {
            if (effects[i].var == lmp->vars[j] && effects[i].val == lmp->vals[j]) {
                correct_effect = true;
                break;
            }
        }
        if (correct_effect)
            // ...if so, test whether the condition can be satisfied
            effect_condition_reachable = _possibly_fires(effects[i].conditions, lvl_var);
        else
            continue;
        if (effect_condition_reachable)
            reaches_lm = true;
    }

    return reaches_lm;
}
