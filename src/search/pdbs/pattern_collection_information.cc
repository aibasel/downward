#include "pattern_collection_information.h"

#include "pattern_database.h"
#include "pdb_max_cliques.h"
#include "validation.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <utility>

using namespace std;

PatternCollectionInformation::PatternCollectionInformation(shared_ptr<AbstractTask> task,
                                     shared_ptr<PatternCollection> patterns)
    : task(task),
      task_proxy(*task),
      patterns(patterns),
      pdbs(nullptr),
      max_additive_subsets(nullptr) {
    assert(patterns);
    validate_and_normalize_patterns(task_proxy, *patterns);
}

PatternCollectionInformation::PatternCollectionInformation(
    shared_ptr<AbstractTask> task,
    shared_ptr<MaxAdditivePDBSubsets> max_additive_subsets)
    : task(task),
      task_proxy(*task),
      patterns(make_shared<PatternCollection>()),
      pdbs(make_shared<PDBCollection>()),
      max_additive_subsets(max_additive_subsets) {
    assert(max_additive_subsets);
    using PatternPDBPair = pair<Pattern, shared_ptr<PatternDatabase>>;
    vector<PatternPDBPair> patterns_and_pdbs;
    unordered_set<Pattern> known_patterns;
    for (const PDBCollection &additive_subset : *max_additive_subsets) {
        for (const shared_ptr<PatternDatabase> &pdb : additive_subset) {
            Pattern pattern = pdb->get_pattern();
            /*
              We use the invariant that there are no two different PDBs with
              the same pattern (since lists of patterns are sorted and unique).
            */
            if (known_patterns.count(pattern) == 0) {
                known_patterns.insert(pattern);
                patterns_and_pdbs.push_back(make_pair(pattern, pdb));
            }
        }
    }

    sort(patterns_and_pdbs.begin(), patterns_and_pdbs.end(),
         [](const PatternPDBPair &pair1, const PatternPDBPair &pair2) {
             return pair1.first < pair2.first;
         });

    for (const PatternPDBPair &pattern_and_pdb : patterns_and_pdbs) {
        patterns->push_back(pattern_and_pdb.first);
        pdbs->push_back(pattern_and_pdb.second);
    }
}

void PatternCollectionInformation::create_pdbs_if_missing() {
    assert(patterns);
    if (!pdbs) {
        pdbs = make_shared<PDBCollection>();
        for (const Pattern &pattern : *patterns) {
            shared_ptr<PatternDatabase> pdb =
                make_shared<PatternDatabase>(task_proxy, pattern);
            pdbs->push_back(pdb);
        }
    }
}

void PatternCollectionInformation::create_max_additive_subsets_if_missing() {
    create_pdbs_if_missing();
    assert(pdbs);
    if (!max_additive_subsets) {
        VariableAdditivity are_additive = compute_additive_vars(task_proxy);
        max_additive_subsets = compute_max_additive_subsets(*pdbs, are_additive);
    }
}

shared_ptr<PatternCollection> PatternCollectionInformation::get_patterns() {
    return patterns;
}

shared_ptr<PDBCollection> PatternCollectionInformation::get_pdbs() {
    create_pdbs_if_missing();
    return pdbs;
}

shared_ptr<MaxAdditivePDBSubsets> PatternCollectionInformation::get_max_additive_subsets() {
    create_max_additive_subsets_if_missing();
    return max_additive_subsets;
}
