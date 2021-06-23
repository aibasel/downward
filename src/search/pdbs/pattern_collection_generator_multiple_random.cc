#include "pattern_collection_generator_multiple_random.h"
#include "pattern_database.h"
#include "rcg.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../task_utils/causal_graph.h"

#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <vector>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorMultipleRandom::PatternCollectionGeneratorMultipleRandom(
    options::Options &opts)
    : PatternCollectionGeneratorMultiple(opts),
      max_pdb_size(opts.get<int>("max_pdb_size")),
      random_causal_graph_max_time(opts.get<double>("max_time")),
      bidirectional(opts.get<bool>("bidirectional")) {
}

string PatternCollectionGeneratorMultipleRandom::get_name() const {
    return "Random Patterns";
}

void PatternCollectionGeneratorMultipleRandom::initialize(
    const shared_ptr<AbstractTask> &task) {
    // Compute CG neighbors once. They will be shuffled after each use.
    cg_neighbors = compute_cg_neighbors(task, bidirectional);
}

PatternInformation PatternCollectionGeneratorMultipleRandom::compute_pattern(
    const shared_ptr<AbstractTask> &task,
    FactPair goal,
    unordered_set<int> &&,
    const utils::CountdownTimer &timer,
    int remaining_collection_size) {
    // TODO: add support for blacklisting in single RCG?
    int remaining_pdb_size = min(remaining_collection_size, max_pdb_size);
    double remaining_time =
        min(static_cast<double>(timer.get_remaining_time()), random_causal_graph_max_time);
    const utils::Verbosity random_causal_graph_verbosity(utils::Verbosity::SILENT);
    Pattern pattern = generate_pattern_rcg(
        remaining_pdb_size,
        remaining_time,
        goal.var,
        random_causal_graph_verbosity,
        rng,
        TaskProxy(*task),
        cg_neighbors
    );

    PatternInformation result(TaskProxy(*task), move(pattern));
    return result;
}

static shared_ptr<PatternCollectionGenerator> _parse(options::OptionParser &parser) {
    add_rcg_options_to_parser(parser);
    add_multiple_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternCollectionGeneratorMultipleRandom>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("random_patterns", _parse);
}
