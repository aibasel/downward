#include "factor_scoring_functions.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../options/option_parser.h"
#include "../options/plugin.h"

#include "../utils/rng_options.h"
#include "../utils/rng.h"

#include <algorithm>

using namespace std;

namespace merge_and_shrink {
vector<int> FactorScoringFunctionInitH::compute_scores(
    const FactoredTransitionSystem &fts,
    const vector<int> &indices) const {
    vector<int> scores;
    scores.reserve(indices.size());
    for (int index : indices) {
        scores.push_back(fts.get_init_state_goal_distance(index));
    }
    return scores;
}

static shared_ptr<FactorScoringFunction> _parse_fsf_inith(options::OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<FactorScoringFunctionInitH>();
}

static options::Plugin<FactorScoringFunction> _plugin_fsf_inith("fsf_inith", _parse_fsf_inith);


vector<int> FactorScoringFunctionSize::compute_scores(
    const FactoredTransitionSystem &fts,
    const vector<int> &indices) const {
    vector<int> scores;
    scores.reserve(indices.size());
    for (int index : indices) {
        scores.push_back(fts.get_transition_system(index).get_size());
    }
    return scores;
}

static shared_ptr<FactorScoringFunction> _parse_fsf_size(options::OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<FactorScoringFunctionSize>();
}

static options::Plugin<FactorScoringFunction> _plugin_fsf_size("fsf_size", _parse_fsf_size);

FactorScoringFunctionRandom::FactorScoringFunctionRandom(
    const options::Options &options) :
    rng(utils::parse_rng_from_options(options)) {
}

vector<int> FactorScoringFunctionRandom::compute_scores(
    const FactoredTransitionSystem &,
    const vector<int> &indices) const {
    vector<int> scores(indices.size());
    iota(scores.begin(), scores.end(), 0);
    rng->shuffle(scores);
    return scores;
}

static shared_ptr<FactorScoringFunction> _parse_fsf_random(options::OptionParser &parser) {
    utils::add_rng_options(parser);
    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<FactorScoringFunctionRandom>(opts);
}

static options::Plugin<FactorScoringFunction> _plugin_fsf_random("fsf_random", _parse_fsf_random);

static options::PluginTypePlugin<FactorScoringFunction> _type_plugin(
    "FactorScoringFunction",
    "This page describes various factor scoring functions. A factor scoring "
    "function, given an FTS and a list of factor indices to be considered, "
    "computes a score for each factor. Maximal scores are considered best.");
}
