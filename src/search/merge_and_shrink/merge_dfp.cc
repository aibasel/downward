#include "merge_scoring_function_dfp.h"
#include "merge_scoring_function_goal_relevance.h"
#include "merge_scoring_function_single_random.h"
#include "merge_scoring_function_total_order.h"
#include "merge_selector_score_based_filtering.h"
#include "merge_strategy_factory_stateless.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/markup.h"

using namespace std;

namespace merge_and_shrink {
static shared_ptr<MergeStrategyFactory>_parse_dfp(options::OptionParser &parser) {
    parser.document_synopsis(
        "Merge strategy DFP",
        "This merge strategy implements the algorithm originally described in the "
        "paper \"Directed model checking with distance-preserving abstractions\" "
        "by Draeger, Finkbeiner and Podelski (SPIN 2006), adapted to planning in "
        "the following paper:" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "Generalized Label Reduction for Merge-and-Shrink Heuristics",
            "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
            "Proceedings of the 28th AAAI Conference on Artificial"
            " Intelligence (AAAI 2014)",
            "2358-2366",
            "AAAI Press 2014"));
    // this also includes the rng option for MergeScoringFunctionSingleRandom.
    MergeScoringFunctionTotalOrder::add_options_to_parser(parser);
    parser.add_option<bool>(
        "randomized_order",
        "If true, use a 'globally' randomized order, i.e. all transition "
        "systems are considered in an arbitrary order. This renders all other "
        "ordering options void.",
        "false");
    options::Options options = parser.parse();
    if (parser.dry_run())
        return nullptr;

    vector<shared_ptr<MergeScoringFunction>> scoring_functions;
    scoring_functions.push_back(make_shared<MergeScoringFunctionGoalRelevance>());
    scoring_functions.push_back(make_shared<MergeScoringFunctionDFP>());

    bool randomized_order = options.get<bool>("randomized_order");
    if (randomized_order) {
        shared_ptr<MergeScoringFunctionSingleRandom> scoring_random =
            make_shared<MergeScoringFunctionSingleRandom>(options);
        scoring_functions.push_back(scoring_random);
    } else {
        shared_ptr<MergeScoringFunctionTotalOrder> scoring_total_order =
            make_shared<MergeScoringFunctionTotalOrder>(options);
        scoring_functions.push_back(scoring_total_order);
    }

    // TODO: the option parser does not handle this
//    options::Options selector_options;
//    selector_options.set<vector<shared_ptr<MergeScoringFunction>>(
//        "scoring_functions",
//        scoring_functions);
    shared_ptr<MergeSelector> selector =
        make_shared<MergeSelectorScoreBasedFiltering>(move(scoring_functions));

    options::Options strategy_options;
    strategy_options.set<shared_ptr<MergeSelector>>(
        "merge_selector",
        selector);

    return make_shared<MergeStrategyFactoryStateless>(strategy_options);
}

static options::PluginShared<MergeStrategyFactory> _plugin_dfp("merge_dfp", _parse_dfp);
}
