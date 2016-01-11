#include "dominance_pruning.h"

#include "pattern_database.h"

#include "../utils/hash.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

namespace pdbs {
using PDBRelation = unordered_set<pair<PatternDatabase *, PatternDatabase *>>;

PDBRelation compute_superset_relation(const PDBCollection &pattern_databases) {
    PDBRelation superset_relation;
    for (const shared_ptr<PatternDatabase> &pdb1 : pattern_databases) {
        const Pattern &pattern1 = pdb1->get_pattern();
        for (const shared_ptr<PatternDatabase> &pdb2 : pattern_databases) {
            const Pattern &pattern2 = pdb2->get_pattern();
            // Note that std::includes assumes that patterns are sorted.
            if (std::includes(pattern1.begin(), pattern1.end(),
                              pattern2.begin(), pattern2.end())) {
                superset_relation.insert(make_pair(pdb1.get(), pdb2.get()));
                /*
                  If we already added the inverse tuple to the relation, the
                  PDBs use the same pattern, which violates the invariant that
                  lists of patterns are sorted and unique.
                */
                assert(pdb1 == pdb2 ||
                       superset_relation.count(make_pair(pdb2.get(),
                                                         pdb1.get())) == 0);
            }
        }
    }
    return superset_relation;
}

bool collection_dominates(const PDBCollection &superset,
                          const PDBCollection &subset,
                          const PDBRelation &superset_relation) {
    for (const shared_ptr<PatternDatabase> &p_subset : subset) {
        // Assume there is no superset until we found one.
        bool found_superset = false;
        for (const shared_ptr<PatternDatabase> &p_superset : superset) {
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

shared_ptr<MaxAdditivePDBSubsets> prune_dominated_subsets(
    const PDBCollection &pattern_databases,
    const MaxAdditivePDBSubsets &max_additive_subsets) {
    utils::Timer timer;
    int num_patterns = pattern_databases.size();
    int num_additive_subsets = max_additive_subsets.size();


    shared_ptr<MaxAdditivePDBSubsets> nondominated_subsets =
        make_shared<MaxAdditivePDBSubsets>();
    /*
      Remember which collections are already removed and don't use them to prune
      other collections. This prevents removing both copies of a collection that
      occurs twice.
    */
    vector<bool> subset_removed(num_additive_subsets, false);

    PDBRelation superset_relation = compute_superset_relation(pattern_databases);
    // Check all pairs of collections for dominance.
    for (int c1_id = 0; c1_id < num_additive_subsets; ++c1_id) {
        const PDBCollection &c1 = max_additive_subsets[c1_id];
        /*
          Collection c1 is useful if it is not dominated by any collection c2.
          Assume that it is useful and set it to false if any dominating
          collection is found.
        */
        bool c1_is_useful = true;
        for (int c2_id = 0; c2_id < num_additive_subsets; ++c2_id) {
            if (c1_id == c2_id || subset_removed[c2_id]) {
                continue;
            }
            const PDBCollection &c2 = max_additive_subsets[c2_id];

            if (collection_dominates(c2, c1, superset_relation)) {
                c1_is_useful = false;
                break;
            }
        }
        if (c1_is_useful) {
            nondominated_subsets->push_back(c1);
        } else {
            subset_removed[c1_id] = true;
        }
    }

    cout << "Pruned " << num_additive_subsets - nondominated_subsets->size() <<
        " of " << num_additive_subsets << " maximal additive subsets" << endl;

    unordered_set<PatternDatabase *> remaining_pdbs;
    for (const PDBCollection &collection : *nondominated_subsets) {
        for (const shared_ptr<PatternDatabase> &pdb : collection) {
            remaining_pdbs.insert(pdb.get());
        }
    }
    cout << "Pruned " << num_patterns - remaining_pdbs.size() <<
        " of " << num_patterns << " PDBs" << endl;

    cout << "Dominance pruning took " << timer << endl;

    return nondominated_subsets;
}
}
