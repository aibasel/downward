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

void MergeScoringFunctionMIASM::shrink_factor(
    TransitionSystem &ts,
    const Distances &dist,
    int new_size,
    Verbosity verbosity) const {
    StateEquivalenceRelation equivalence_relation =
        shrink_strategy->compute_equivalence_relation(ts, dist, new_size);
    // TODO: We currently violate this; see issue250
    //assert(equivalence_relation.size() <= target_size);
    int new_num_states = equivalence_relation.size();
    if (new_num_states < ts.get_size()) {
        /* Compute the abstraction mapping based on the given state equivalence
           relation. */
        vector<int> abstraction_mapping = compute_abstraction_mapping(
            ts.get_size(), equivalence_relation);
        ts.apply_abstraction(
            equivalence_relation, abstraction_mapping, verbosity);
        // Not applying abstraction to distances because we don't need to.
    }
}

vector<double> MergeScoringFunctionMIASM::compute_scores(
    const FactoredTransitionSystem &fts,
    const vector<pair<int, int>> &merge_candidates) {
    vector<double> scores;
    scores.reserve(merge_candidates.size());
    for (pair<int, int> merge_candidate : merge_candidates) {
        int index1 = merge_candidate.first;
        int index2 = merge_candidate.second;

        // Copy the transition systems for further processing.
        TransitionSystem ts1(fts.get_ts(index1));
        TransitionSystem ts2(fts.get_ts(index2));

        // Imitate shrinking and merging as done in the merge-and-shrink loop.
        pair<int, int> new_sizes = compute_shrink_sizes(
            ts1.get_size(),
            ts2.get_size(),
            max_states_before_merge,
            max_states);
        Verbosity verbosity = Verbosity::SILENT;
        if (ts1.get_size() > min(new_sizes.first, shrink_threshold_before_merge)) {
            Distances dist1(ts1, fts.get_distances(index1));
            shrink_factor(ts1, dist1, new_sizes.first, verbosity);
        }
        if (ts2.get_size() > min(new_sizes.second, shrink_threshold_before_merge)) {
            Distances dist2(ts2, fts.get_distances(index2));
            shrink_factor(ts2, dist2, new_sizes.second, verbosity);
        }

        /*
          Compute the product of the two copied transition systems and compute
          distance information to count the number of alive states.
        */
        unique_ptr<TransitionSystem> product = TransitionSystem::merge(
            fts.get_labels(), ts1, ts2, verbosity);
        unique_ptr<Distances> dist = utils::make_unique_ptr<Distances>(*product);
        const bool compute_init_distances = true;
        const bool compute_goal_distances = true;
        dist->compute_distances(compute_init_distances, compute_goal_distances, verbosity);
        int num_states = product->get_size();
        int alive_states_count = 0;
        for (int state = 0; state < num_states; ++state) {
            if (dist->get_init_distance(state) != INF &&
                dist->get_goal_distance(state) != INF) {
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

static options::PluginShared<MergeScoringFunction> _plugin("miasm", _parse);
}
