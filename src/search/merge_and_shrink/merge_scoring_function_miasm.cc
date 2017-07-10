#include "merge_scoring_function_miasm.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "merge_and_shrink_heuristic.h"
#include "shrink_strategy.h"
#include "transition_system.h"
#include "utils.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

using namespace std;

namespace merge_and_shrink {
MergeScoringFunctionMIASM::MergeScoringFunctionMIASM(
    const options::Options &options)
    : shrink_strategy(options.get<shared_ptr<ShrinkStrategy>>("shrink_strategy")),
      max_states(options.get<int>("max_states")),
      max_states_before_merge(options.get<int>("max_states_before_merge")),
      shrink_threshold_before_merge(options.get<int>("threshold_before_merge")) {
}

vector<double> MergeScoringFunctionMIASM::compute_scores(
    const FactoredTransitionSystem &fts,
    const vector<pair<int, int>> &merge_candidates) {
    vector<double> scores;
    scores.reserve(merge_candidates.size());
    for (pair<int, int> merge_candidate : merge_candidates) {
        int index1 = merge_candidate.first;
        int index2 = merge_candidate.second;
        unique_ptr<TransitionSystem> product = shrink_before_merge_externally(
            fts,
            index1,
            index2,
            *shrink_strategy,
            max_states,
            max_states_before_merge,
            shrink_threshold_before_merge);

        // Compute distances for the product and count the alive states.
        unique_ptr<Distances> distances = utils::make_unique_ptr<Distances>(*product);
        const bool compute_init_distances = true;
        const bool compute_goal_distances = true;
        const Verbosity verbosity = Verbosity::SILENT;
        distances->compute_distances(compute_init_distances, compute_goal_distances, verbosity);
        int num_states = product->get_size();
        int alive_states_count = 0;
        for (int state = 0; state < num_states; ++state) {
            if (distances->get_init_distance(state) != INF &&
                distances->get_goal_distance(state) != INF) {
                ++alive_states_count;
            }
        }

        /*
          Compute the score as the ratio of alive states of the product
          compared to the number of states of the full product.

          HACK! The version used in the paper accidentally used the sizes of
          the non-shrunk transition systems for computing the ratio of
          pruned states in the product. We emulate this behavior here.
        */
        num_states = fts.get_ts(index1).get_size() * fts.get_ts(index2).get_size();
        double score = static_cast<double>(alive_states_count) /
                       static_cast<double>(num_states);
        scores.push_back(score);
    }
    return scores;
}

string MergeScoringFunctionMIASM::name() const {
    return "miasm";
}

static shared_ptr<MergeScoringFunction>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "miasm",
        "This scoring function favors merging transition systems such that in "
        "their product, there are many dead states, which can then be pruned "
        "without sacrificing information. To do so, for every candidate pair "
        "of transition systems, this class copies the two transition systems, "
        "possibly shrinks them (using the same shrink strategy as the overall "
        "merge-and-shrink computation), and then computes their product. The "
        "score for the merge candidate is the ratio of alive states of this "
        "product compared to the number of states in the full product.");
    parser.document_note(
        "miasm()",
        "We recommend using full pruning, i.e. pruning of both unreachable "
        "and irrelevant states, when using this merge scoring function, so "
        "that the ranking of merge candidates based on the ratio of alive "
        "to all states actually corresponds to the amount of pruning that "
        "happens after choosing a merge candidate.");

    // TODO: use shrink strategy and limit options from MergeAndShrinkHeuristic
    // instead of having the identical options here again.
    parser.add_option<shared_ptr<ShrinkStrategy>>(
        "shrink_strategy",
        "The given shrink strategy configuration should match the one "
        "given to merge_and_shrink.");
    MergeAndShrinkHeuristic::add_shrink_limit_options_to_parser(parser);

    options::Options options = parser.parse();
    MergeAndShrinkHeuristic::handle_shrink_limit_options_defaults(options);

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeScoringFunctionMIASM>(options);
}

static options::PluginShared<MergeScoringFunction> _plugin("sf_miasm", _parse);
}
