#include "pattern_collection_generator_multiple_cegar.h"

#include "cegar.h"
#include "pattern_database.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/countdown_timer.h"
#include "../utils/markup.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <vector>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorMultipleCegar::PatternCollectionGeneratorMultipleCegar(
    options::Options &opts)
    : PatternCollectionGeneratorMultiple(opts),
      max_pdb_size(opts.get<int>("max_pdb_size")),
      cegar_max_time(opts.get<double>("max_time")),
      use_wildcard_plans(opts.get<bool>("use_wildcard_plans")) {
}

string PatternCollectionGeneratorMultipleCegar::get_name() const {
    return "Multiple CEGAR";
}

PatternInformation PatternCollectionGeneratorMultipleCegar::compute_pattern(
    const shared_ptr<AbstractTask> &task,
    FactPair goal,
    unordered_set<int> &&blacklisted_variables,
    const utils::CountdownTimer &timer,
    int remaining_collection_size) {
    int remaining_pdb_size = min(remaining_collection_size, max_pdb_size);
    double remaining_time =
        min(static_cast<double>(timer.get_remaining_time()), cegar_max_time);
    const utils::Verbosity cegar_verbosity(utils::Verbosity::SILENT);
    /*
      Call CEGAR with the remaining size budget (limiting one of PDB and
      collection size would be enough, but this is cleaner).
    */
    CEGAR cegar(
        remaining_pdb_size,
        remaining_collection_size,
        use_wildcard_plans,
        remaining_time,
        cegar_verbosity,
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
        "described in the paper" + utils::format_conference_reference(
            {"Alexander Rovner", "Silvan Sievers", "Malte Helmert"},
            "Counterexample-Guided Abstraction Refinement for Pattern Selection "
            "in Optimal Classical Planning",
            "https://ai.dmi.unibas.ch/papers/rovner-et-al-icaps2019.pdf",
            "Proceedings of the 29th International Conference on Automated "
            "Planning and Scheduling (ICAPS 2019)",
            "362-367",
            "AAAI Press",
            "2019"));
    add_implementation_notes_to_parser(parser);
    add_multiple_options_to_parser(parser);
    // TODO: this adds max_pdb_size and max_collection_size twice and does
    // not allow using different size limits for the single and multiple CEGAR
    // algorithms. But maybe this is also not desired.
    add_cegar_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternCollectionGeneratorMultipleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("multiple_cegar", _parse);
}
