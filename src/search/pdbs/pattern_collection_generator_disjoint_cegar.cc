#include "pattern_collection_generator_disjoint_cegar.h"

#include "cegar.h"
#include "utils.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/rng_options.h"

using namespace std;

namespace pdbs {
PatternCollectionGeneratorDisjointCegar::PatternCollectionGeneratorDisjointCegar(
    const plugins::Options &opts)
    : PatternCollectionGenerator(opts),
      max_pdb_size(opts.get<int>("max_pdb_size")),
      max_collection_size(opts.get<int>("max_collection_size")),
      max_time(opts.get<double>("max_time")),
      use_wildcard_plans(opts.get<bool>("use_wildcard_plans")),
      rng(utils::parse_rng_from_options(opts)) {
}

string PatternCollectionGeneratorDisjointCegar::name() const {
    return "disjoint CEGAR pattern collection generator";
}

PatternCollectionInformation PatternCollectionGeneratorDisjointCegar::compute_patterns(
    const shared_ptr<AbstractTask> &task) {
    // Store the set of goals in random order.
    TaskProxy task_proxy(*task);
    vector<FactPair> goals = get_goals_in_random_order(task_proxy, *rng);

    return generate_pattern_collection_with_cegar(
        max_pdb_size,
        max_collection_size,
        max_time,
        use_wildcard_plans,
        log,
        rng,
        task,
        move(goals));
}

class PatternCollectionGeneratorDisjointCegarFeature : public plugins::TypedFeature<PatternCollectionGenerator, PatternCollectionGeneratorDisjointCegar> {
public:
    PatternCollectionGeneratorDisjointCegarFeature() : TypedFeature("disjoint_cegar") {
        document_title("Disjoint CEGAR");
        document_synopsis(
            "This pattern collection generator uses the CEGAR algorithm to "
            "compute a pattern for the planning task. See below "
            "for a description of the algorithm and some implementation notes. "
            "The original algorithm (called single CEGAR) is described in the "
            "paper " + get_rovner_et_al_reference());

        // TODO: these options could be move to the base class; see issue1022.
        add_option<int>(
            "max_pdb_size",
            "maximum number of states per pattern database (ignored for the "
            "initial collection consisting of a singleton pattern for each goal "
            "variable)",
            "1000000",
            plugins::Bounds("1", "infinity"));
        add_option<int>(
            "max_collection_size",
            "maximum number of states in the pattern collection (ignored for the "
            "initial collection consisting of a singleton pattern for each goal "
            "variable)",
            "10000000",
            plugins::Bounds("1", "infinity"));
        add_option<double>(
            "max_time",
            "maximum time in seconds for this pattern collection generator "
            "(ignored for computing the initial collection consisting of a "
            "singleton pattern for each goal variable)",
            "infinity",
            plugins::Bounds("0.0", "infinity"));
        add_cegar_wildcard_option_to_feature(*this);
        add_generator_options_to_feature(*this);
        utils::add_rng_options(*this);

        add_cegar_implementation_notes_to_feature(*this);
    }
};

static plugins::FeaturePlugin<PatternCollectionGeneratorDisjointCegarFeature> _plugin;
}
