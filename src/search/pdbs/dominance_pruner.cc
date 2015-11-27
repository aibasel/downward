#include "dominance_pruner.h"

#include "pattern_database.h"

#include <algorithm>
#include <vector>

using namespace std;


DominancePruner::DominancePruner(PDBCollection &pattern_databases,
                                 PDBCliques &max_cliques)
    : pattern_databases(pattern_databases),
      max_cliques(max_cliques) {
}

void DominancePruner::compute_superset_relation() {
    superset_relation.clear();
    for (const auto &pdb1 : pattern_databases) {
        const Pattern &pattern1 = pdb1->get_pattern();
        for (const auto &pdb2 : pattern_databases) {
            const Pattern &pattern2 = pdb2->get_pattern();
            // Note that std::includes assumes that patterns are sorted.
            if (std::includes(pattern1.begin(), pattern1.end(),
                              pattern2.begin(), pattern2.end())) {
                superset_relation.insert(make_pair(pdb1.get(), pdb2.get()));
                /*
                  If we already added the inverse tuple to the relation, the
                  PDBs use the same pattern and we only need one of them.
                  This should not happen, but in case it does we can easily fix
                  it by replacing one of the PDBs in all cliques that use it.
                */
                if (superset_relation.count(make_pair(pdb2.get(),
                                                      pdb1.get()))) {
                    replace_pdb(pdb2, pdb1);
                }
            }
        }
    }
}

void DominancePruner::replace_pdb(shared_ptr<PatternDatabase> old_pdb,
                                  shared_ptr<PatternDatabase> new_pdb) {
    for (auto &collection : max_cliques) {
        for (size_t p_id = 0; p_id < collection.size(); ++p_id) {
            if (collection[p_id] == old_pdb) {
                collection[p_id] = new_pdb;
            }
        }
    }
}

bool DominancePruner::clique_dominates(const PDBCollection &superset,
                                       const PDBCollection &subset) {
    for (const auto &p_subset : subset) {
        // Assume there is no superset until we found one.
        bool found_superset = false;
        for (const auto &p_superset : superset) {
            if (superset_relation.count(make_pair(p_superset.get(),
                                                  p_subset.get()))) {
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
    PDBCliques remaining_cliques;
    /*
      Remember which cliques are already removed and don't use them to prune
      other cliques. This prevents removing both copies of a clique that occurs
      twice.
    */
    vector<bool> clique_removed(max_cliques.size(), false);
    unordered_set<shared_ptr<PatternDatabase>> remaining_heuristics;

    compute_superset_relation();
    // Check all pairs of cliques for dominance.
    for (size_t c1_id = 0; c1_id < max_cliques.size(); ++c1_id) {
        PDBCollection &c1 = max_cliques[c1_id];
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
            PDBCollection &c2 = max_cliques[c2_id];

            if (clique_dominates(c2, c1)) {
                c1_is_useful = false;
                break;
            }
        }
        if (c1_is_useful) {
            remaining_cliques.push_back(c1);
            for (const auto &pdb : c1) {
                remaining_heuristics.insert(pdb);
            }
        } else {
            clique_removed[c1_id] = true;
        }
    }
    /* TODO issue585: this does not maintain the invariant that lists of
       patterns are sorted. Should we sort the list before returning? */
    pattern_databases = PDBCollection(remaining_heuristics.begin(),
                                      remaining_heuristics.end());
    max_cliques.swap(remaining_cliques);
}
