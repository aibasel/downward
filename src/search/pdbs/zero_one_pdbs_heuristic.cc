#include "zero_one_pdbs_heuristic.h"

#include "pattern_generator.h"

#include "../plugins/plugin.h"

#include <limits>

using namespace std;

namespace pdbs {
ZeroOnePDBs get_zero_one_pdbs_from_options(
    const shared_ptr<AbstractTask> &task, const plugins::Options &opts) {
    shared_ptr<PatternCollectionGenerator> pattern_generator =
        opts.get<shared_ptr<PatternCollectionGenerator>>("patterns");
    PatternCollectionInformation pattern_collection_info =
        pattern_generator->generate(task);
    shared_ptr<PatternCollection> patterns =
        pattern_collection_info.get_patterns();
    TaskProxy task_proxy(*task);
    return ZeroOnePDBs(task_proxy, *patterns);
}

ZeroOnePDBsHeuristic::ZeroOnePDBsHeuristic(
    const plugins::Options &opts)
    : Heuristic(opts),
      zero_one_pdbs(get_zero_one_pdbs_from_options(task, opts)) {
}

int ZeroOnePDBsHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int h = zero_one_pdbs.get_value(state);
    if (h == numeric_limits<int>::max())
        return DEAD_END;
    return h;
}

class ZeroOnePDBsHeuristicFeature : public plugins::TypedFeature<Evaluator, ZeroOnePDBsHeuristic> {
public:
    ZeroOnePDBsHeuristicFeature() : TypedFeature("zopdbs") {
        document_subcategory("heuristics_pdb");
        document_title("Zero-One PDB");
        document_synopsis(
            "The zero/one pattern database heuristic is simply the sum of the "
            "heuristic values of all patterns in the pattern collection. In contrast "
            "to the canonical pattern database heuristic, there is no need to check "
            "for additive subsets, because the additivity of the patterns is "
            "guaranteed by action cost partitioning. This heuristic uses the most "
            "simple form of action cost partitioning, i.e. if an operator affects "
            "more than one pattern in the collection, its costs are entirely taken "
            "into account for one pattern (the first one which it affects) and set "
            "to zero for all other affected patterns.");

        add_option<shared_ptr<PatternCollectionGenerator>>(
            "patterns",
            "pattern generation method",
            "systematic(1)");
        Heuristic::add_options_to_feature(*this);

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "yes");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }
};

static plugins::FeaturePlugin<ZeroOnePDBsHeuristicFeature> _plugin;
}
