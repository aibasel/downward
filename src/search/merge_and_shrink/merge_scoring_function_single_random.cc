#include "merge_scoring_function_single_random.h"

#include "types.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <cassert>

using namespace std;

namespace merge_and_shrink {
MergeScoringFunctionSingleRandom::MergeScoringFunctionSingleRandom(
    const options::Options &options)
    : random_seed(options.get<int>("random_seed")),
      rng(utils::parse_rng_from_options(options)) {
}

vector<double> MergeScoringFunctionSingleRandom::compute_scores(
    const FactoredTransitionSystem &,
    const vector<pair<int, int>> &merge_candidates) {
    int chosen_index = (*rng)(merge_candidates.size());
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

void MergeScoringFunctionSingleRandom::dump_function_specific_options() const {
    cout << "Random seed: " << random_seed << endl;
}

static shared_ptr<MergeScoringFunction>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Single random",
        "This scoring function assigns exactly one merge candidate a score of "
        "0, chosen randomly, and infinity to all others.");
    utils::add_rng_options(parser);

    options::Options options = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeScoringFunctionSingleRandom>(options);
}

static options::PluginShared<MergeScoringFunction> _plugin("single_random", _parse);
}
