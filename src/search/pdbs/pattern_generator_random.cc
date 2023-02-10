#include "pattern_generator_random.h"

#include "pattern_database.h"
#include "random_pattern.h"
#include "utils.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <vector>

using namespace std;

namespace pdbs {
PatternGeneratorRandom::PatternGeneratorRandom(const plugins::Options &opts)
    : PatternGenerator(opts),
      max_pdb_size(opts.get<int>("max_pdb_size")),
      max_time(opts.get<double>("max_time")),
      bidirectional(opts.get<bool>("bidirectional")),
      rng(utils::parse_rng_from_options(opts)) {
}

string PatternGeneratorRandom::name() const {
    return "random pattern generator";
}

PatternInformation PatternGeneratorRandom::compute_pattern(
    const shared_ptr<AbstractTask> &task) {
    vector<vector<int>> cg_neighbors = compute_cg_neighbors(
        task, bidirectional);
    TaskProxy task_proxy(*task);
    vector<FactPair> goals = get_goals_in_random_order(task_proxy, *rng);

    Pattern pattern = generate_random_pattern(
        max_pdb_size,
        max_time,
        log,
        rng,
        task_proxy,
        goals[0].var,
        cg_neighbors);

    return PatternInformation(task_proxy, pattern, log);
}

class PatternGeneratorRandomFeature : public plugins::TypedFeature<PatternGenerator, PatternGeneratorRandom> {
public:
    PatternGeneratorRandomFeature() : TypedFeature("random_pattern") {
        document_title("Random Pattern");
        document_synopsis(
            "This pattern generator implements the 'single randomized "
            "causal graph' algorithm described in experiments of the the paper"
            + get_rovner_et_al_reference() +
            "See below for a description of the algorithm and some implementation "
            "notes.");

        add_option<int>(
            "max_pdb_size",
            "maximum number of states in the final pattern database (possibly "
            "ignored by a singleton pattern consisting of a single goal variable)",
            "1000000",
            plugins::Bounds("1", "infinity"));
        add_option<double>(
            "max_time",
            "maximum time in seconds for the pattern generation",
            "infinity",
            plugins::Bounds("0.0", "infinity"));
        add_random_pattern_bidirectional_option_to_feature(*this);
        add_generator_options_to_feature(*this);
        utils::add_rng_options(*this);

        add_random_pattern_implementation_notes_to_feature(*this);
    }
};

static plugins::FeaturePlugin<PatternGeneratorRandomFeature> _plugin;
}
