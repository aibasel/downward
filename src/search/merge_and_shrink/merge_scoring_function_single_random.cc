#include "merge_scoring_function_single_random.h"

#include "types.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <cassert>

using namespace std;

namespace merge_and_shrink {
MergeScoringFunctionSingleRandom::MergeScoringFunctionSingleRandom(
    int random_seed)
    : random_seed(random_seed),
      rng(utils::get_rng(random_seed)) {
}

vector<double> MergeScoringFunctionSingleRandom::compute_scores(
    const FactoredTransitionSystem &,
    const vector<pair<int, int>> &merge_candidates) {
    int chosen_index = rng->random(merge_candidates.size());
    vector<double> scores;
    scores.reserve(merge_candidates.size());
    for (size_t candidate_index = 0; candidate_index < merge_candidates.size();
         ++candidate_index) {
        if (static_cast<int>(candidate_index) == chosen_index) {
            scores.push_back(0);
        } else {
            scores.push_back(INF);
        }
    }
    return scores;
}

string MergeScoringFunctionSingleRandom::name() const {
    return "single random";
}

void MergeScoringFunctionSingleRandom::dump_function_specific_options(
    utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        log << "Random seed: " << random_seed << endl;
    }
}

class MergeScoringFunctionSingleRandomFeature
    : public plugins::TypedFeature<MergeScoringFunction, MergeScoringFunctionSingleRandom> {
public:
    MergeScoringFunctionSingleRandomFeature() : TypedFeature("single_random") {
        document_title("Single random");
        document_synopsis(
            "This scoring function assigns exactly one merge candidate a score of "
            "0, chosen randomly, and infinity to all others.");

        utils::add_rng_options_to_feature(*this);
    }

    virtual shared_ptr<MergeScoringFunctionSingleRandom>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<MergeScoringFunctionSingleRandom>(
            utils::get_rng_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<MergeScoringFunctionSingleRandomFeature> _plugin;
}
