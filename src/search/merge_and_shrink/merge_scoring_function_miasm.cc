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

#include "../utils/markup.h"

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
        */
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
        "product compared to the number of states in the full product."
        "The merge strategy using this scoring function, called dyn-MIASM, "
        "is described in the following (and see the note below for "
        "corresponding configurations):"
        + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
            "http://ai.cs.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
            "Proceedings of the 26th International Conference on Planning and "
            "Scheduling (ICAPS 2016)",
            "2358-2366",
            "AAAI Press 2016"));
    parser.document_note(
        "Note",
        "To obtain the configurations called dyn-MIASM described in the paper, "
        "use this scoring function within a stateless merge strategy using a "
        "score based filtering approach, with additional tie-breaking, i.e.: "
        "{{{merge_strategy=merge_stateless(merge_selector=score_based_filtering"
        "(scoring_functions=[sf_miasm,total_order]))}}}");
    parser.document_note(
        "Note",
        "Reasonable configurations use the same options related to shrinking"
        "for miasm as for merge_and_shrink, i.e. the options {{{"
        "shrink_strategy}}}, {{{max_states}}}, and {{{threshold_before_merge}}} "
        "should be set identically. Furthermore, as this scoring functions "
        "maximizes the amount of possible pruning, merge-and-shrink should be "
        "configured to use full pruning, i.e. {{{prune_unreachable_states=true"
        "}}} and {{{prune_irrelevant_states=true}}} (the default).");

    // TODO: use shrink strategy and limit options from MergeAndShrinkHeuristic
    // instead of having the identical options here again.
    parser.add_option<shared_ptr<ShrinkStrategy>>(
        "shrink_strategy",
        "The given shrink strategy configuration should match the one "
        "given to {{{merge_and_shrink}}}, cf the note below.");
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
