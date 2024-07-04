#include "canonical_pdbs_heuristic.h"

#include "dominance_pruning.h"
#include "utils.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/timer.h"

#include <iostream>
#include <limits>
#include <memory>

using namespace std;

namespace pdbs {
static CanonicalPDBs get_canonical_pdbs(
    const shared_ptr<AbstractTask> &task,
    const shared_ptr<PatternCollectionGenerator> &pattern_generator,
    double max_time_dominance_pruning, utils::LogProxy &log) {
    utils::Timer timer;
    if (log.is_at_least_normal()) {
        log << "Initializing canonical PDB heuristic..." << endl;
    }
    PatternCollectionInformation pattern_collection_info =
        pattern_generator->generate(task);
    shared_ptr<PatternCollection> patterns =
        pattern_collection_info.get_patterns();
    /*
      We compute PDBs and pattern cliques here (if they have not been
      computed before) so that their computation is not taken into account
      for dominance pruning time.
    */
    shared_ptr<PDBCollection> pdbs = pattern_collection_info.get_pdbs();
    shared_ptr<vector<PatternClique>> pattern_cliques =
        pattern_collection_info.get_pattern_cliques();

    if (max_time_dominance_pruning > 0.0) {
        int num_variables = TaskProxy(*task).get_variables().size();
        /*
          NOTE: Dominance pruning could also be computed without having access
          to the PDBs, but since we want to delete patterns, we also want to
          update the list of corresponding PDBs so they are synchronized.

          In the long term, we plan to have patterns and their PDBs live
          together, in which case we would only need to pass their container
          and the pattern cliques.
        */
        prune_dominated_cliques(
            *patterns,
            *pdbs,
            *pattern_cliques,
            num_variables,
            max_time_dominance_pruning,
            log);
    }

    dump_pattern_collection_generation_statistics(
        "Canonical PDB heuristic", timer(), pattern_collection_info, log);
    return CanonicalPDBs(pdbs, pattern_cliques);
}

CanonicalPDBsHeuristic::CanonicalPDBsHeuristic(
    const shared_ptr<PatternCollectionGenerator> &patterns,
    double max_time_dominance_pruning,
    const shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(transform, cache_estimates, description, verbosity),
      canonical_pdbs(
          get_canonical_pdbs(
              task, patterns, max_time_dominance_pruning, log)) {
}

int CanonicalPDBsHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int h = canonical_pdbs.get_value(state);
    if (h == numeric_limits<int>::max()) {
        return DEAD_END;
    } else {
        return h;
    }
}

void add_canonical_pdbs_options_to_feature(plugins::Feature &feature) {
    feature.add_option<double>(
        "max_time_dominance_pruning",
        "The maximum time in seconds spent on dominance pruning. Using 0.0 "
        "turns off dominance pruning. Dominance pruning excludes patterns "
        "and additive subsets that will never contribute to the heuristic "
        "value because there are dominating subsets in the collection.",
        "infinity",
        plugins::Bounds("0.0", "infinity"));
}

tuple<double> get_canonical_pdbs_arguments_from_options(
    const plugins::Options &opts) {
    return make_tuple(opts.get<double>("max_time_dominance_pruning"));
}

class CanonicalPDBsHeuristicFeature
    : public plugins::TypedFeature<Evaluator, CanonicalPDBsHeuristic> {
public:
    CanonicalPDBsHeuristicFeature() : TypedFeature("cpdbs") {
        document_subcategory("heuristics_pdb");
        document_title("Canonical PDB");
        document_synopsis(
            "The canonical pattern database heuristic is calculated as follows. "
            "For a given pattern collection C, the value of the "
            "canonical heuristic function is the maximum over all "
            "maximal additive subsets A in C, where the value for one subset "
            "S in A is the sum of the heuristic values for all patterns in S "
            "for a given state.");

        add_option<shared_ptr<PatternCollectionGenerator>>(
            "patterns",
            "pattern generation method",
            "systematic(1)");
        add_canonical_pdbs_options_to_feature(*this);
        add_heuristic_options_to_feature(*this, "cpdbs");

        document_language_support("action costs", "supported");
        document_language_support("conditional effects", "not supported");
        document_language_support("axioms", "not supported");

        document_property("admissible", "yes");
        document_property("consistent", "yes");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<CanonicalPDBsHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<CanonicalPDBsHeuristic>(
            opts.get<shared_ptr<PatternCollectionGenerator>>(
                "patterns"),
            get_canonical_pdbs_arguments_from_options(opts),
            get_heuristic_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<CanonicalPDBsHeuristicFeature> _plugin;
}
