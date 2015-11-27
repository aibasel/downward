#include "pattern_collection.h"

#include "pattern_database.h"
#include "pdb_max_cliques.h"
#include "util.h"

#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <utility>

using namespace std;

PatternCollection::PatternCollection(shared_ptr<AbstractTask> task,
                                     shared_ptr<Patterns> patterns)
    : task(task),
      task_proxy(*task),
      patterns(patterns),
      pdbs(nullptr),
      cliques(nullptr) {
    assert(patterns);
    validate_and_normalize_patterns(task_proxy, *patterns);
}

PatternCollection::PatternCollection(shared_ptr<AbstractTask> task,
                                     shared_ptr<PDBCliques> cliques)
    : task(task),
      task_proxy(*task),
      patterns(make_shared<Patterns>()),
      pdbs(make_shared<PDBCollection>()),
      cliques(cliques) {
    assert(cliques);
    using PatternPDBPair = pair<Pattern, shared_ptr<PatternDatabase>>;
    vector<PatternPDBPair> patterns_and_pdbs;
    unordered_set<Pattern> known_patterns;
    for (const auto &clique : *cliques) {
        for (const auto &pdb : clique) {
            Pattern pattern = pdb->get_pattern();
            // We assume that there are no two different PDBs with the same Pattern.
            // TODO issue585: should we test for the implicit assumption above?
            if (known_patterns.count(pattern) == 0) {
                known_patterns.insert(pattern);
                /* TODO issue585: Do we have to validate/normalize the pattern
                   here? Since it comes from a PDB, it should already be
                   normalized, but we don't know if it comes from the correct
                   task. */
                validate_and_normalize_pattern(task_proxy, pattern);
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

void PatternCollection::create_pdbs_if_missing() {
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

void PatternCollection::create_cliques_if_missing() {
    create_pdbs_if_missing();
    assert(pdbs);
    if (!cliques) {
        VariableAdditivity are_additive = compute_additive_vars(task_proxy);
        cliques = compute_max_pdb_cliques(*pdbs, are_additive);
    }
}

shared_ptr<Patterns> PatternCollection::get_patterns() {
    return patterns;
}

shared_ptr<PDBCollection> PatternCollection::get_pdbs() {
    create_pdbs_if_missing();
    return pdbs;
}

shared_ptr<PDBCliques> PatternCollection::get_cliques() {
    create_cliques_if_missing();
    return cliques;
}
