#include "canonical_pdbs_heuristic.h"

#include "pattern_generator.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../timer.h"

using namespace std;


CanonicalPDBs get_canonical_pdbs_from_options(
    const shared_ptr<AbstractTask> task, const Options &opts) {
    shared_ptr<PatternCollectionGenerator> pattern_generator =
        opts.get<shared_ptr<PatternCollectionGenerator>>("patterns");
    Timer timer;
    PatternCollection pattern_collection = pattern_generator->generate(task);
    shared_ptr<PDBCollection> pattern_databases = pattern_collection.get_pdbs();
    shared_ptr<PDBCliques> max_cliques = pattern_collection.get_cliques();
    cout << "PDB collection construction time: " << timer << endl;

    CanonicalPDBs canonical_pdbs(pattern_databases, max_cliques);
    if (opts.get<bool>("dominance_pruning")) {
        canonical_pdbs.dominance_pruning();
    }
    return canonical_pdbs;
}

CanonicalPDBsHeuristic::CanonicalPDBsHeuristic(const Options &opts)
    : Heuristic(opts),
      canonical_pdbs(get_canonical_pdbs_from_options(task, opts)) {
}

int CanonicalPDBsHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    return compute_heuristic(state);
}

int CanonicalPDBsHeuristic::compute_heuristic(const State &state) const {
    int h = canonical_pdbs.get_value(state);
    if (h == numeric_limits<int>::max()) {
        return DEAD_END;
    } else {
        return h;
    }
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
    parser.add_option<bool>(
        "dominance_pruning",
        "Exclude patterns and cliques that will never contribute to the "
        "heuristic value because there are dominating patterns in the "
        "collection.",
        "true");

    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return new CanonicalPDBsHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cpdbs", _parse);
