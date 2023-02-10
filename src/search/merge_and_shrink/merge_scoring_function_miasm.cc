#include "merge_scoring_function_miasm.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "merge_and_shrink_algorithm.h"
#include "shrink_strategy.h"
#include "transition_system.h"
#include "merge_scoring_function_miasm_utils.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/markup.h"

using namespace std;

namespace merge_and_shrink {
MergeScoringFunctionMIASM::MergeScoringFunctionMIASM(
    const plugins::Options &options)
    : shrink_strategy(options.get<shared_ptr<ShrinkStrategy>>("shrink_strategy")),
      max_states(options.get<int>("max_states")),
      max_states_before_merge(options.get<int>("max_states_before_merge")),
      shrink_threshold_before_merge(options.get<int>("threshold_before_merge")),
      silent_log(utils::get_silent_log()) {
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
            shrink_threshold_before_merge,
            silent_log);

        // Compute distances for the product and count the alive states.
        unique_ptr<Distances> distances = utils::make_unique_ptr<Distances>(*product);
        const bool compute_init_distances = true;
        const bool compute_goal_distances = true;
        distances->compute_distances(compute_init_distances, compute_goal_distances, silent_log);
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
        assert(num_states);
        double score = static_cast<double>(alive_states_count) /
            static_cast<double>(num_states);
        scores.push_back(score);
    }
    return scores;
}

string MergeScoringFunctionMIASM::name() const {
    return "miasm";
}

class MergeScoringFunctionMIASMFeature : public plugins::TypedFeature<MergeScoringFunction, MergeScoringFunctionMIASM> {
public:
    MergeScoringFunctionMIASMFeature() : TypedFeature("sf_miasm") {
        document_title("MIASM");
        document_synopsis(
            "This scoring function favors merging transition systems such that in "
            "their product, there are many dead states, which can then be pruned "
            "without sacrificing information. In particular, the score it assigns "
            "to a product is the ratio of alive states to the total number of "
            "states. To compute this score, this class thus computes the product "
            "of all pairs of transition systems, potentially copying and shrinking "
            "the transition systems before if otherwise their product would exceed "
            "the specified size limits. A stateless merge strategy using this "
            "scoring function is called dyn-MIASM (nowadays also called sbMIASM "
            "for score-based MIASM) and is described in the following paper:"
            + utils::format_conference_reference(
                {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
                "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
                "https://ai.dmi.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
                "Proceedings of the 26th International Conference on Planning and "
                "Scheduling (ICAPS 2016)",
                "2358-2366",
                "AAAI Press",
                "2016"));

        // TODO: use shrink strategy and limit options from MergeAndShrinkHeuristic
        // instead of having the identical options here again.
        add_option<shared_ptr<ShrinkStrategy>>(
            "shrink_strategy",
            "We recommend setting this to match the shrink strategy configuration "
            "given to {{{merge_and_shrink}}}, see note below.");
        add_transition_system_size_limit_options_to_feature(*this);

        document_note(
            "Note",
            "To obtain the configurations called dyn-MIASM described in the paper, "
            "use the following configuration of the merge-and-shrink heuristic "
            "and adapt the tie-breaking criteria of {{{total_order}}} as desired:\n"
            "{{{\nmerge_and_shrink(merge_strategy=merge_stateless(merge_selector="
            "score_based_filtering(scoring_functions=[sf_miasm(shrink_strategy="
            "shrink_bisimulation(greedy=false),max_states=50000,"
            "threshold_before_merge=1),total_order(atomic_ts_order=reverse_level,"
            "product_ts_order=new_to_old,atomic_before_product=true)])),"
            "shrink_strategy=shrink_bisimulation(greedy=false),label_reduction="
            "exact(before_shrinking=true,before_merging=false),max_states=50000,"
            "threshold_before_merge=1)\n}}}");
        document_note(
            "Note",
            "Unless you know what you are doing, we recommend using the same "
            "options related to shrinking for {{{sf_miasm}}} as for {{{"
            "merge_and_shrink}}}, i.e. the options {{{shrink_strategy}}}, {{{"
            "max_states}}}, and {{{threshold_before_merge}}} should be set "
            "identically. Furthermore, as this scoring function maximizes the "
            "amount of possible pruning, merge-and-shrink should be configured to "
            "use full pruning, i.e. {{{prune_unreachable_states=true}}} and {{{"
            "prune_irrelevant_states=true}}} (the default).");
    }

    virtual shared_ptr<MergeScoringFunctionMIASM> create_component(const plugins::Options &options, const utils::Context &context) const override {
        plugins::Options options_copy(options);
        handle_shrink_limit_options_defaults(options_copy, context);
        return make_shared<MergeScoringFunctionMIASM>(options_copy);
    }
};

static plugins::FeaturePlugin<MergeScoringFunctionMIASMFeature> _plugin;
}
