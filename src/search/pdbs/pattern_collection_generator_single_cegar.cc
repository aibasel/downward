#include "pattern_collection_generator_single_cegar.h"

#include "cegar.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/markup.h"
#include "../utils/rng_options.h"

using namespace std;

namespace pdbs {
PatternCollectionGeneratorSingleCegar::PatternCollectionGeneratorSingleCegar(
    const options::Options &opts)
    : max_pdb_size(opts.get<int>("max_pdb_size")),
      max_collection_size(opts.get<int>("max_collection_size")),
      wildcard_plans(opts.get<bool>("wildcard_plans")),
      max_time(opts.get<double>("max_time")),
      verbosity(opts.get<utils::Verbosity>("verbosity")),
      rng(utils::parse_rng_from_options(opts)) {
}

PatternCollectionInformation PatternCollectionGeneratorSingleCegar::generate(
    const shared_ptr<AbstractTask> &task) {
    // Store the set of goals in random order.
    TaskProxy task_proxy(*task);
    vector<FactPair> goals = get_goals_in_random_order(task_proxy, rng);

    return CEGAR(
        max_pdb_size,
        max_collection_size,
        wildcard_plans,
        max_time,
        verbosity,
        rng,
        task,
        move(goals)).compute_pattern_collection();
}

static shared_ptr<PatternCollectionGenerator> _parse(
    options::OptionParser &parser) {
    parser.document_synopsis(
        "Single CEGAR",
        "This pattern collection generator implements the single CEGAR algorithm "
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
    add_cegar_options_to_parser(parser);
    utils::add_verbosity_option_to_parser(parser);
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorSingleCegar>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("single_cegar", _parse);
}
