#include "pattern_generator.h"

#include "pattern_database.h"

#include "../plugin.h"

#include <cassert>
#include <unordered_set>

using namespace std;

PatternCollection::PatternCollection(
    shared_ptr<AbstractTask> task, shared_ptr<Patterns> patterns,
    shared_ptr<PDBCollection> pdbs, shared_ptr<PDBCliques> cliques)
    : task(task),
      task_proxy(*task),
      patterns(patterns),
      pdbs(pdbs),
      cliques(cliques) {
    if (!patterns && !pdbs && !cliques) {
        // Empty pattern collection
        patterns = make_shared<Patterns>();
    }
}

PatternCollection::~PatternCollection() {
}

void PatternCollection::extract_patterns() {
    assert(patterns || pdbs || cliques);
    if (patterns) {
        return;
    } else if (pdbs) {
        patterns = make_shared<Patterns>();
        for (const auto &pdb : *pdbs) {
            patterns->push_back(pdb->get_pattern());
        }
    } else if (cliques) {
        /*
          We extract pdbs and patterns at the same time, to guarantee that each
          index points to corresponding elements in both vectors. Creating the
          vector pdbs here is no significant overhead, because the PDBs are
          already created and we only copy pointers to them.
        */
        patterns = make_shared<Patterns>();
        pdbs = make_shared<PDBCollection>();
        unordered_set<Pattern> known_patterns;
        for (const auto &clique : *cliques) {
            for (const auto &pdb : clique) {
                Pattern pattern = pdb->get_pattern();
                if (known_patterns.count(pattern) == 0) {
                    known_patterns.insert(pattern);
                    patterns->push_back(pattern);
                    pdbs->push_back(pdb);
                }
            }
        }
    }
}

void PatternCollection::construct_pdbs() {
    pdbs = make_shared<PDBCollection>();
    for (const Pattern &pattern : *get_patterns()) {
        shared_ptr<PatternDatabase> pdb = make_shared<PatternDatabase>(task_proxy, pattern);
        pdbs->push_back(pdb);
    }
}

void PatternCollection::construct_cliques() {
    // TODO issue585: construct maximal additive subsets of patterns
}

shared_ptr<Patterns> PatternCollection::get_patterns() {
    if (!patterns) {
        extract_patterns();
    }
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


static PluginTypePlugin<PatternCollectionGenerator> _type_plugin_collection(
    "PatternCollectionGenerator",
    "Factory for pattern collections");

static PluginTypePlugin<PatternGenerator> _type_plugin_single(
    "PatternGenerator",
    "Factory for single patterns");
