#include "pattern_generator.h"

#include "pattern_database.h"

#include <cassert>

using namespace std;

PatternCollection::PatternCollection(
    shared_ptr<AbstractTask> task, shared_ptr<Patterns> patterns,
    shared_ptr<PDBCollection> pdbs, shared_ptr<PDBCliques> cliques)
    : task(task),
      task_proxy(*task),
      patterns(patterns),
      pdbs(pdbs),
      cliques(cliques) {
}

PatternCollection::~PatternCollection() {
}

void PatternCollection::construct_pdbs() {
    pdbs = make_shared<PDBCollection>();
    for (const Pattern &pattern : *patterns) {
        shared_ptr<PatternDatabase> pdb = make_shared<PatternDatabase>(task_proxy, pattern);
        pdbs->push_back(pdb);
    }
}

void PatternCollection::construct_cliques() {
    assert(patterns);
    // TODO issue585: construct maximal additive subsets of patterns
}

shared_ptr<Patterns> PatternCollection::get_patterns() {
    assert(patterns);
    return patterns;
}

shared_ptr<PDBCollection> PatternCollection::get_pdbs() {
    if (!pdbs) {
        construct_pdbs();
    }
    return pdbs;
}

shared_ptr<PDBCliques> PatternCollection::get_cliques() {
    if (!cliques) {
        construct_cliques();
    }
    return cliques;
}
