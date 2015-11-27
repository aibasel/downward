#include "incremental_canonical_pdbs.h"

#include "canonical_pdbs.h"
#include "pattern_database.h"

#include "../timer.h"
#include "../utilities.h"

#include <cstdlib>
#include <iostream>
#include <limits>

using namespace std;

IncrementalCanonicalPDBs::IncrementalCanonicalPDBs(
    const shared_ptr<AbstractTask> task, const Patterns &intitial_patterns)
    : task(task),
      task_proxy(*task),
      pattern_databases(make_shared<PDBCollection>()),
      max_cliques(nullptr),
      size(0) {
    Timer timer;
    pattern_databases->reserve(intitial_patterns.size());
    for (const Pattern &pattern : intitial_patterns)
        add_pdb_for_pattern(pattern);
    are_additive = compute_additive_vars(task_proxy);
    recompute_max_cliques();
    cout << "PDB collection construction time: " << timer << endl;
}

void IncrementalCanonicalPDBs::add_pdb_for_pattern(const Pattern &pattern) {
    pattern_databases->emplace_back(new PatternDatabase(task_proxy, pattern));
    size += pattern_databases->back()->get_size();
}

void IncrementalCanonicalPDBs::add_pattern(const Pattern &pattern) {
    add_pdb_for_pattern(pattern);
    recompute_max_cliques();
}

void IncrementalCanonicalPDBs::recompute_max_cliques() {
    max_cliques = compute_max_pdb_cliques(*pattern_databases, are_additive);
}

PDBCliques IncrementalCanonicalPDBs::get_max_additive_subsets(
    const Pattern &new_pattern) {
    return ::get_max_additive_subsets(*max_cliques, new_pattern, are_additive);
}

int IncrementalCanonicalPDBs::get_value(const State &state) const {
    CanonicalPDBs canonical_pdbs(pattern_databases, max_cliques);
    return canonical_pdbs.get_value(state);
}

bool IncrementalCanonicalPDBs::is_dead_end(const State &state) const {
    for (const auto &pdb : *pattern_databases)
        if (pdb->get_value(state) == numeric_limits<int>::max())
            return true;
    return false;
}
