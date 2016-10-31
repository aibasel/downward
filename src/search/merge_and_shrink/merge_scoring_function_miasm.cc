#include "merge_scoring_function_miasm.h"

#include "factored_transition_system.h"
#include "merge_and_shrink_heuristic.h"
//#include "shrink_strategy.h"
#include "transition_system.h"
#include "utils.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

using namespace std;

namespace merge_and_shrink {
MergeScoringFunctionMIASM::MergeScoringFunctionMIASM(
    const options::Options &options)
    : shrink_stratey(options.get<shared_ptr<ShrinkStrategy>>("shrink_strategy")),
      max_states(options.get<int>("max_states")),
      max_states_before_merge(options.get<int>("max_states_before_merge")),
      shrink_threshold_before_merge(options.get<int>("threshold_before_merge")) {
}

vector<double> MergeScoringFunctionMIASM::compute_scores(
    FactoredTransitionSystem &fts,
    const vector<pair<int, int>> &merge_candidates) {
    vector<double> scores;
    scores.reserve(merge_candidates.size());
    for (pair<int, int> merge_candidate : merge_candidates ) {
        int ts_index1 = merge_candidate.first;
        int ts_index2 = merge_candidate.second;

        int merge_index = shrink_and_merge_temporarily(
            fts, ts_index1, ts_index2, *shrink_stratey, max_states,
            max_states_before_merge, shrink_threshold_before_merge);

        // return 0 if the merge is unsolvable (i.e. empty transition system)
        double score = 0;
        if (fts.get_ts(merge_index).is_solvable()) {
            int expected_size = fts.get_ts(ts_index1).get_size() *
                fts.get_ts(ts_index2).get_size();
            assert(expected_size);
            int new_size = fts.get_ts(merge_index).get_size();
            assert(new_size <= expected_size);
            score = static_cast<double>(new_size) /
                static_cast<double>(expected_size);
        }
        scores.push_back(score);

        // delete the merge and reset
        fts.release_copies();
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
        "The given shrink stratgy configuration should match the one "
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
