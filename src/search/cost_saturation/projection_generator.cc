#include "projection_generator.h"

#include "projection.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../pdbs/dominance_pruning.h"
#include "../pdbs/pattern_database.h"
#include "../pdbs/pattern_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

#include <memory>

using namespace pdbs;
using namespace std;

namespace cost_saturation {
ProjectionGenerator::ProjectionGenerator(const options::Options &opts)
    : pattern_generator(
          opts.get<shared_ptr<pdbs::PatternCollectionGenerator>>("patterns")),
      dominance_pruning(opts.get<bool>("dominance_pruning")),
      use_add_after_delete_semantics(opts.get<bool>("use_add_after_delete_semantics")),
      debug(opts.get<bool>("debug")) {
}

Abstractions ProjectionGenerator::generate_abstractions(
    const shared_ptr<AbstractTask> &task) {
    utils::Timer patterns_timer;
    utils::Log log;
    TaskProxy task_proxy(*task);

    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);

    log << "Compute patterns" << endl;
    PatternCollectionInformation pattern_collection_info =
        pattern_generator->generate(task);
    shared_ptr<pdbs::PatternCollection> patterns =
        pattern_collection_info.get_patterns();

    log << "Number of patterns: " << patterns->size() << endl;
    log << "Time for computing patterns: " << patterns_timer << endl;

    if (dominance_pruning) {
        utils::Timer pdb_timer;
        shared_ptr<PDBCollection> pdbs = pattern_collection_info.get_pdbs();
        shared_ptr<MaxAdditivePDBSubsets> max_additive_subsets =
            pattern_collection_info.get_max_additive_subsets();
        cout << "PDB construction time: " << pdb_timer << endl;
        utils::Timer pruning_timer;
        max_additive_subsets = prune_dominated_subsets(
            *pdbs, *max_additive_subsets, task_proxy.get_variables().size(),
            numeric_limits<double>::infinity());
        cout << "Dominance pruning time: " << pruning_timer << endl;
        patterns->clear();
        for (const auto &subset : *max_additive_subsets) {
            for (const shared_ptr<PatternDatabase> &pdb : subset) {
                patterns->push_back(pdb->get_pattern());
            }
        }
        cout << "Number of non-dominated patterns: " << patterns->size() << endl;
    }

    log << "Build projections" << endl;
    utils::Timer pdbs_timer;
    Abstractions abstractions;
    for (const pdbs::Pattern &pattern : *patterns) {
        if (debug) {
            log << "Pattern " << abstractions.size() + 1 << ": "
                << pattern << endl;
        }
        abstractions.push_back(
            utils::make_unique_ptr<Projection>(task_proxy, pattern));
        if (debug) {
            abstractions.back()->dump();
        }
    }
    log << "Time for building projections: " << pdbs_timer << endl;
    log << "Number of projections: " << abstractions.size() << endl;
    return abstractions;
}

static shared_ptr<AbstractionGenerator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Projection generator",
        "");

    parser.add_option<shared_ptr<pdbs::PatternCollectionGenerator>>(
        "patterns",
        "pattern generation method",
        OptionParser::NONE);
    parser.add_option<bool>(
        "dominance_pruning",
        "prune dominated patterns",
        "false");
    parser.add_option<bool>(
        "use_add_after_delete_semantics",
        "skip transitions that are invalid according to add-after-delete semantics",
        "false");
    parser.add_option<bool>(
        "debug",
        "print debugging info",
        "false");

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<ProjectionGenerator>(opts);
}

static PluginShared<AbstractionGenerator> _plugin("projections", _parse);
}
