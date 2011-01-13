#include "util.h"

#include "landmark_graph.h"

#include "../operator.h"

#include <limits>

using namespace std;
using namespace __gnu_cxx;

bool _possibly_fires(const vector<Prevail> &prevail, const vector<vector<int> > &lvl_var) {
    for (int j = 0; j < prevail.size(); j++)
        if (lvl_var[prevail[j].var][prevail[j].prev] ==
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

    // Test whether all "prevail" preconditions of o can be reached
    // Otherwise, operator is not applicable
    const vector<Prevail> &prevail = o.get_prevail();
    for (unsigned i = 0; i < prevail.size(); i++)
        if (lvl_var[prevail[i].var][prevail[i].prev] ==
            numeric_limits<int>::max())
            return false;

    // Test remaining preconditions and conditions in effects
    bool reaches_lm = false;
    // Go through all effects of o...
    const vector<PrePost> &prepost = o.get_pre_post();
    for (unsigned i = 0; i < prepost.size(); i++) {
        // If there is a precondition on the effect, check whether it can be reached
        // Otherwise, operator is not applicable
        if (prepost[i].pre != -1)
            if (lvl_var[prepost[i].var][prepost[i].pre] ==
                numeric_limits<int>::max())
                return false;

        // If lmp is a conditional effect, check the condition
        bool effect_condition_reachable = false;
        assert(!lvl_var[prepost[i].var].empty());
        bool correct_effect = false;
        // Check whether *this* effect of o reaches a proposition in lmp...
        for (unsigned int j = 0; j < lmp->vars.size(); j++) {
            if (prepost[i].var == lmp->vars[j] && prepost[i].post == lmp->vals[j]) {
                correct_effect = true;
                break;
            }
        }
        if (correct_effect)
            // ...if so, test whether the condition can be satisfied
            effect_condition_reachable = _possibly_fires(prepost[i].cond, lvl_var);
        else
            continue;
        if (effect_condition_reachable)
            reaches_lm = true;
    }

    return reaches_lm;
}
