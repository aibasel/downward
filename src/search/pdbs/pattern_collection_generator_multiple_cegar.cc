#include "pattern_collection_generator_multiple_cegar.h"

#include "cegar.h"
#include "pattern_database.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"

using namespace std;

namespace pdbs {
PatternCollectionGeneratorMultipleCegar::PatternCollectionGeneratorMultipleCegar(
    options::Options &opts)
    : PatternCollectionGeneratorMultiple(opts, "Multiple CEGAR"),
      use_wildcard_plans(opts.get<bool>("use_wildcard_plans")) {
}

PatternInformation PatternCollectionGeneratorMultipleCegar::compute_pattern(
    int max_pdb_size,
    double max_time,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    FactPair goal,
    unordered_set<int> &&blacklisted_variables) {
    /*
      Call CEGAR with the remaining size budget (limiting one of PDB and
      collection size would be enough, but this is cleaner).
    */
    CEGAR cegar(
        max_pdb_size,
        max_pdb_size,
        max_time,
        use_wildcard_plans,
        utils::Verbosity::SILENT,
        rng,
        task,
        {goal},
        move(blacklisted_variables));
    PatternCollectionInformation collection_info =
        cegar.compute_pattern_collection();

    shared_ptr<PatternCollection> new_patterns = collection_info.get_patterns();
    if (new_patterns->size() > 1) {
        cerr << "CEGAR limited to one goal computed more than one pattern" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }

    Pattern &pattern = new_patterns->front();
    shared_ptr<PDBCollection> new_pdbs = collection_info.get_pdbs();
    shared_ptr<PatternDatabase> &pdb = new_pdbs->front();
    PatternInformation result(TaskProxy(*task), move(pattern));
    result.set_pdb(pdb);
    return result;
}

static shared_ptr<PatternCollectionGenerator> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Multiple CEGAR",
        "This pattern collection generator implements the multiple CEGAR algorithm "
        "described in the paper" + get_rovner_et_al_reference());
    add_implementation_notes_to_parser(parser);
    add_multiple_options_to_parser(parser);
    add_cegar_wildcard_option_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternCollectionGeneratorMultipleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("multiple_cegar", _parse);
}
