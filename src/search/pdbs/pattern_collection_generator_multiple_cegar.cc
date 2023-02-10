#include "pattern_collection_generator_multiple_cegar.h"

#include "cegar.h"
#include "pattern_database.h"
#include "utils.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

using namespace std;

namespace pdbs {
PatternCollectionGeneratorMultipleCegar::PatternCollectionGeneratorMultipleCegar(
    const plugins::Options &opts)
    : PatternCollectionGeneratorMultiple(opts),
      use_wildcard_plans(opts.get<bool>("use_wildcard_plans")) {
}

string PatternCollectionGeneratorMultipleCegar::id() const {
    return "CEGAR";
}

PatternInformation PatternCollectionGeneratorMultipleCegar::compute_pattern(
    int max_pdb_size,
    double max_time,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    const shared_ptr<AbstractTask> &task,
    const FactPair &goal,
    unordered_set<int> &&blacklisted_variables) {
    utils::LogProxy silent_log = utils::get_silent_log();
    return generate_pattern_with_cegar(
        max_pdb_size,
        max_time,
        use_wildcard_plans,
        silent_log,
        rng,
        task,
        goal,
        move(blacklisted_variables));
}

class PatternCollectionGeneratorMultipleCegarFeature : public plugins::TypedFeature<PatternCollectionGenerator, PatternCollectionGeneratorMultipleCegar> {
public:
    PatternCollectionGeneratorMultipleCegarFeature() : TypedFeature("multiple_cegar") {
        document_title("Multiple CEGAR");
        document_synopsis(
            "This pattern collection generator implements the multiple CEGAR "
            "algorithm described in the paper" + get_rovner_et_al_reference() +
            "It is an instantiation of the 'multiple algorithm framework'. "
            "To compute a pattern in each iteration, it uses the CEGAR algorithm "
            "restricted to a single goal variable. See below for descriptions of "
            "the algorithms.");

        add_multiple_options_to_feature(*this);
        add_cegar_wildcard_option_to_feature(*this);

        add_cegar_implementation_notes_to_feature(*this);
        add_multiple_algorithm_implementation_notes_to_feature(*this);
    }
};

static plugins::FeaturePlugin<PatternCollectionGeneratorMultipleCegarFeature> _plugin;
}
