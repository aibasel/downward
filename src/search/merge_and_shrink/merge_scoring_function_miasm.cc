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
        TransitionSystem ts1(fts.get_ts(index1));
        TransitionSystem ts2(fts.get_ts(index2));
        /*
          Compute the size limit for both transition systems as imposed by
          max_states and max_states_before_merge.
        */
        pair<int, int> new_sizes = compute_shrink_sizes(
            ts1.get_size(),
            ts2.get_size(),
            max_states_before_merge,
            max_states);

        /*
          For both transition systems, possibly compute and apply an
          abstraction.
          TODO: we could better use the given limit by increasing the size limit
          for the second shrinking if the first shrinking was larger than
          required.
        */
        Verbosity verbosity = Verbosity::SILENT;

        if (ts1.get_size() > min(new_sizes.first, shrink_threshold_before_merge)) {
            Distances dist1(ts1, fts.get_distances(index1));
            shrink_factor(ts1, dist1, new_sizes.first, verbosity);
        }

        if (ts2.get_size() > min(new_sizes.second, shrink_threshold_before_merge)) {
            Distances dist2(ts2, fts.get_distances(index2));
            shrink_factor(ts2, dist2, new_sizes.second, verbosity);
        }

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
        // HACK! the previous version had a bug which we emulate here:
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
        "This scoring function computes the actual product (after applying "
        "shrinking before merging) for every candidate merge and then computes "
        "the ratio of its real size to its expected size if the full product "
        "was computed. This ratio immediately corresponds to the score, hence "
        "preferring merges where more pruning of unreachable/irrelevant states "
        "and/or perfect happened.");

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
