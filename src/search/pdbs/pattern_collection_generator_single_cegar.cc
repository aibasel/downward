#include "pattern_collection_generator_single_cegar.h"

#include "cegar.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/rng_options.h"

using namespace std;

namespace pdbs {
PatternCollectionGeneratorSingleCegar::PatternCollectionGeneratorSingleCegar(
    const options::Options &opts)
    : max_refinements(opts.get<int>("max_refinements")),
      max_pdb_size(opts.get<int>("max_pdb_size")),
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
    vector<FactPair> goals;
    for (FactProxy goal : task_proxy.get_goals()) {
        goals.push_back(goal.get_pair());
    }
    rng->shuffle(goals);

    return CEGAR(
        max_refinements,
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
