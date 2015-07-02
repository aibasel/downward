#include "dominance_pruner.h"

#include <algorithm>

using namespace std;

DominancePruner::DominancePruner(vector<PatternDatabase *> &pattern_databases_,
                                 vector<vector<PatternDatabase *> > &max_cliques_)
    : pattern_databases(pattern_databases_),
      max_cliques(max_cliques_) {
}

void DominancePruner::compute_superset_relation() {
    superset_relation.clear();
    size_t num_patterns = pattern_databases.size();
    for (size_t i = 0; i < num_patterns; ++i) {
        const vector<int> p_i = pattern_databases[i]->get_pattern();
        for (size_t j = 0; j < num_patterns; ++j) {
            const vector<int> p_j = pattern_databases[j]->get_pattern();
            if (std::includes(p_i.begin(), p_i.end(), p_j.begin(), p_j.end())) {
                superset_relation.insert(make_pair(pattern_databases[i],
                                                   pattern_databases[j]));
                /*
                  If we already added the inverse tuple to the relation, the
                  PDBs use the same pattern and we only need one of them.
                  This should not happen, but in case it does we can easily fix
                  it by replacing one of the PDBs in all cliques that use it.
                */
                if (superset_relation.count(make_pair(pattern_databases[j],
                                                      pattern_databases[i]))) {
                    replace_pdb(pattern_databases[j], pattern_databases[i]);
                }
            }
        }
    }
}

void DominancePruner::replace_pdb(PatternDatabase *old_pdb,
                                  PatternDatabase *new_pdb) {
    for (size_t c_id = 0; c_id < max_cliques.size(); ++c_id) {
        for (size_t p_id = 0; p_id < max_cliques[c_id].size(); ++p_id) {
            if (max_cliques[c_id][p_id] == old_pdb) {
                max_cliques[c_id][p_id] = new_pdb;
            }
        }
    }
}

bool DominancePruner::clique_dominates(const vector<PatternDatabase *> &c_sup,
                                       const vector<PatternDatabase *> &c_sub) {
    /* c_sup dominates c_sub iff for every pattern p_sub in c_sub there is a
       pattern p_sup in c_sup where p_sup is a superset of p_sub. */
    for (size_t p_sub_id = 0; p_sub_id < c_sub.size(); ++p_sub_id) {
        // Assume there is no superset until we found one.
        bool found_superset = false;
        for (size_t p_sup_id = 0; p_sup_id < c_sup.size(); ++p_sup_id) {
            if (superset_relation.count(make_pair(c_sup[p_sup_id],
                                                  c_sub[p_sub_id]))) {
                found_superset = true;
                break;
            }
        }
        if (!found_superset) {
            return false;
        }
    }
    return true;
}


void DominancePruner::prune() {
    vector<vector<PatternDatabase *> > remaining_cliques;
    /*
      Remember which cliques are already removed and don't use them to prune
      other cliques. This prevents removing both copies of a clique that occurs
      twice.
    */
    vector<bool> clique_removed(max_cliques.size(), false);
    unordered_set<PatternDatabase * > remaining_heuristics;

    compute_superset_relation();
    // Check all pairs of cliques for dominance.
    for (size_t c1_id = 0; c1_id < max_cliques.size(); ++c1_id) {
        vector<PatternDatabase *> &c1 = max_cliques[c1_id];
        /*
          Clique c1 is useful if it is not dominated by any clique c2.
          Assume that it is useful and set it to false if any dominating
          clique is found.
        */
        bool c1_is_useful = true;
        for (size_t c2_id = 0; c2_id < max_cliques.size(); ++c2_id) {
            if (c1_id == c2_id || clique_removed[c2_id]) {
                continue;
            }
            vector<PatternDatabase *> &c2 = max_cliques[c2_id];

            if (clique_dominates(c2, c1)) {
                c1_is_useful = false;
                break;
            }
        }
        if (c1_is_useful) {
            remaining_cliques.push_back(c1);
            for (size_t p_id = 0; p_id < c1.size(); ++p_id) {
                PatternDatabase *pdb = c1[p_id];
                remaining_heuristics.insert(pdb);
            }
        } else {
            clique_removed[c1_id] = true;
        }
    }
    pattern_databases = vector<PatternDatabase *>(remaining_heuristics.begin(),
                                                  remaining_heuristics.end());
    max_cliques.swap(remaining_cliques);
}
