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
    const FactPair &goal,
    unordered_set<int> &&blacklisted_variables) {
    return generate_pattern_with_cegar(
        max_pdb_size,
        max_time,
        use_wildcard_plans,
        utils::Verbosity::SILENT,
        rng,
        task,
        goal,
        move(blacklisted_variables));
}

static shared_ptr<PatternCollectionGenerator> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Multiple CEGAR",
        "This pattern collection generator implements the multiple CEGAR "
        "algorithm described in the paper" + get_rovner_et_al_reference() +
        "It is an instantiation of the 'multiple algorithm framework'. "
        "To compute a pattern in each iteration, it uses the CEGAR algorithm "
        "restricted to a single goal variable. See below for descriptions of "
        "the algorithms.");
    add_cegar_implementation_notes_to_parser(parser);
    add_multiple_algorithm_implementation_notes_to_parser(parser);
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
