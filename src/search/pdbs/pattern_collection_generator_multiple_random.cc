#include "pattern_collection_generator_multiple_random.h"

#include "random_pattern.h"
#include "utils.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <vector>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorMultipleRandom::PatternCollectionGeneratorMultipleRandom(
    const plugins::Options &opts)
    : PatternCollectionGeneratorMultiple(opts),
      bidirectional(opts.get<bool>("bidirectional")) {
}

string PatternCollectionGeneratorMultipleRandom::id() const {
    return "random patterns";
}

void PatternCollectionGeneratorMultipleRandom::initialize(
    const shared_ptr<AbstractTask> &task) {
    // Compute CG neighbors once. They are shuffled when used.
    cg_neighbors = compute_cg_neighbors(task, bidirectional);
}

PatternInformation PatternCollectionGeneratorMultipleRandom::compute_pattern(
    int max_pdb_size,
    double max_time,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    const FactPair &goal,
    unordered_set<int> &&) {
    // TODO: add support for blacklisting in single RCG?
    utils::LogProxy silent_log = utils::get_silent_log();
    Pattern pattern = generate_random_pattern(
        max_pdb_size,
        max_time,
        silent_log,
        rng,
        TaskProxy(*task),
        goal.var,
        cg_neighbors);

    PatternInformation result(TaskProxy(*task), move(pattern), log);
    return result;
}

class PatternCollectionGeneratorMultipleRandomFeature : public plugins::TypedFeature<PatternCollectionGenerator, PatternCollectionGeneratorMultipleRandom> {
public:
    PatternCollectionGeneratorMultipleRandomFeature() : TypedFeature("random_patterns") {
        document_title("Multiple Random Patterns");
        document_synopsis(
            "This pattern collection generator implements the 'multiple "
            "randomized causal graph' (mRCG) algorithm described in experiments of "
            "the paper" + get_rovner_et_al_reference() +
            "It is an instantiation of the 'multiple algorithm framework'. "
            "To compute a pattern in each iteration, it uses the random "
            "pattern algorithm, called 'single randomized causal graph' (sRCG) "
            "in the paper. See below for descriptions of the algorithms.");

        add_multiple_options_to_feature(*this);
        add_random_pattern_bidirectional_option_to_feature(*this);

        add_random_pattern_implementation_notes_to_feature(*this);
        add_multiple_algorithm_implementation_notes_to_feature(*this);
    }
};

static plugins::FeaturePlugin<PatternCollectionGeneratorMultipleRandomFeature> _plugin;
}
