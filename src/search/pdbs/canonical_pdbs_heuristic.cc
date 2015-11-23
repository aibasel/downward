#include "canonical_pdbs_heuristic.h"

#include "dominance_pruner.h"
#include "pattern_database.h"
#include "pattern_generator.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../timer.h"

#include <cassert>
#include <cstdlib>

using namespace std;

CanonicalPDBsHeuristic::CanonicalPDBsHeuristic(const Options &opts)
    : Heuristic(opts) {

    shared_ptr<PatternCollectionGenerator> pattern_generator =
        opts.get<shared_ptr<PatternCollectionGenerator>>("patterns");
    Timer timer;
    PatternCollection pattern_collection = pattern_generator->generate(task);
    pattern_databases = pattern_collection.get_pdbs();
    max_cliques = pattern_collection.get_cliques();
    cout << "PDB collection construction time: " << timer << endl;
}

void CanonicalPDBsHeuristic::dominance_pruning() {
    Timer timer;
    int num_patterns = pattern_databases->size();
    int num_cliques = max_cliques->size();

    DominancePruner(*pattern_databases, *max_cliques).prune();

    cout << "Pruned " << num_cliques - max_cliques->size() <<
        " of " << num_cliques << " cliques" << endl;
    cout << "Pruned " << num_patterns - pattern_databases->size() <<
        " of " << num_patterns << " PDBs" << endl;

    cout << "Dominance pruning took " << timer << endl;
}

int CanonicalPDBsHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    return compute_heuristic(state);
}

int CanonicalPDBsHeuristic::compute_heuristic(const State &state) const {
    // If we have an empty collection, then max_cliques = { \emptyset }.
    assert(!max_cliques->empty());
    int max_h = 0;
    for (const auto &clique : *max_cliques) {
        int clique_h = 0;
        for (const auto &pdb : clique) {
            /* Experiments showed that it is faster to recompute the
               h values than to cache them in an unordered_map. */
            int h = pdb->get_value(state);
            if (h == numeric_limits<int>::max())
                return DEAD_END;
            clique_h += h;
        }
        max_h = max(max_h, clique_h);
    }
    return max_h;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Canonical PDB",
        "The canonical pattern database heuristic is calculated as follows. "
        "For a given pattern collection C, the value of the "
        "canonical heuristic function is the maximum over all "
        "maximal additive subsets A in C, where the value for one subset "
        "S in A is the sum of the heuristic values for all patterns in S "
        "for a given state.");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    parser.add_option<shared_ptr<PatternCollectionGenerator>>(
        "patterns",
        "pattern generation method",
        "combo()");
    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return new CanonicalPDBsHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cpdbs", _parse);
