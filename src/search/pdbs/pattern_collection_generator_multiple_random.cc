#include "pattern_collection_generator_multiple_random.h"

#include "random_pattern.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"
#include "../utils/markup.h"

#include <vector>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorMultipleRandom::PatternCollectionGeneratorMultipleRandom(
    options::Options &opts)
    : PatternCollectionGeneratorMultiple(opts),
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
    int max_pdb_size,
    double max_time,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    FactPair goal,
    unordered_set<int> &&) {
    // TODO: add support for blacklisting in single RCG?
    Pattern pattern = generate_random_pattern(
        max_pdb_size,
        max_time,
        utils::Verbosity::SILENT,
        rng,
        TaskProxy(*task),
        goal.var,
        cg_neighbors);

    PatternInformation result(TaskProxy(*task), move(pattern));
    return result;
}

static shared_ptr<PatternCollectionGenerator> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Multiple Random Patterns",
        "This pattern collection generator implements the 'multiple "
        "randomized causal graph' (mRCG) algorithm described in experiments of "
        "the paper" + utils::format_conference_reference(
            {"Alexander Rovner", "Silvan Sievers", "Malte Helmert"},
            "Counterexample-Guided Abstraction Refinement for Pattern Selection "
            "in Optimal Classical Planning",
            "https://ai.dmi.unibas.ch/papers/rovner-et-al-icaps2019.pdf",
            "Proceedings of the 29th International Conference on Automated "
            "Planning and Scheduling (ICAPS 2019)",
            "362-367",
            "AAAI Press",
            "2019") +
        "To compute a pattern in each iteration, it uses the 'single random "
        "pattern' algorithm, called 'single randomized causal graph' (sRCG) "
        "in the paper; see PatternCollectionGenerator#Single_Random_Pattern.");
    add_multiple_options_to_parser(parser);
    add_random_pattern_bidirectional_option_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<PatternCollectionGeneratorMultipleRandom>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("random_patterns", _parse);
}
