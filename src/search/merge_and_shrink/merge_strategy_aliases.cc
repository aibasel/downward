#include "merge_scoring_function_dfp.h"
#include "merge_scoring_function_goal_relevance.h"
#include "merge_scoring_function_single_random.h"
#include "merge_scoring_function_total_order.h"
#include "merge_selector_score_based_filtering.h"
#include "merge_strategy_factory_precomputed.h"
#include "merge_strategy_factory_stateless.h"
#include "merge_tree_factory_linear.h"

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
            "https://ai.dmi.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
            "Proceedings of the 28th AAAI Conference on Artificial"
            " Intelligence (AAAI 2014)",
            "2358-2366",
            "AAAI Press 2014") +
        "Using this command line option is deprecated, please use the "
        "equivalent configurations\n"
        "{{{\nmerge_strategy=merge_stateless(merge_selector=score_based_filtering("
        "scoring_functions=[goal_relevance,dfp,total_order(<order_option>))]))\n}}}\n"
        "if specifying tie-breaking order criteria, or\n"
        "{{{\nmerge_strategy=merge_stateless(merge_selector=score_based_filtering("
        "scoring_functions=[goal_relevance,dfp,single_random()]))\n}}}\n"
        "if using full random tie-breaking.");
    // This also includes the rng option for MergeScoringFunctionSingleRandom.
    MergeScoringFunctionTotalOrder::add_options_to_parser(parser);
    parser.add_option<bool>(
        "randomized_order",
        "If true, use a 'globally' randomized order, i.e. all transition "
        "systems are considered in an arbitrary order. This renders all other "
        "ordering options void.",
        "false");
    if (parser.dry_run() && !parser.help_mode())
        cout << "Warning: this command line option has been deprecated. Please "
            "consult fast-downward.org for equivalent new command line options."
             << endl;

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

static options::Plugin<MergeStrategyFactory> _plugin_dfp("merge_dfp", _parse_dfp);

static shared_ptr<MergeStrategyFactory> _parse_linear(
    options::OptionParser &parser) {
    MergeTreeFactoryLinear::add_options_to_parser(parser);
    parser.document_synopsis(
        "Linear merge strategies",
        "These merge strategies implement several linear merge orders, which "
        "are described in the paper:" + utils::format_paper_reference(
            {"Malte Helmert", "Patrik Haslum", "Joerg Hoffmann"},
            "Flexible Abstraction Heuristics for Optimal Sequential Planning",
            "https://ai.dmi.unibas.ch/papers/helmert-et-al-icaps2007.pdf",
            "Proceedings of the Seventeenth International Conference on"
            " Automated Planning and Scheduling (ICAPS 2007)",
            "176-183",
            "2007") +
        "Using this command line option is deprecated, please use the "
        "equivalent configuration\n"
        "{{{\nmerge_strategy=merge_precomputed(merge_tree=linear(<variable_order>))\n}}}");
    if (parser.dry_run() && !parser.help_mode())
        cout << "Warning: this command line option has been deprecated. Please "
            "consult fast-downward.org for equivalent new command line options."
             << endl;

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    shared_ptr<MergeTreeFactoryLinear> linear_tree_factory =
        make_shared<MergeTreeFactoryLinear>(opts);

    options::Options strategy_factory_options;
    strategy_factory_options.set<shared_ptr<MergeTreeFactory>>(
        "merge_tree", linear_tree_factory);

    return make_shared<MergeStrategyFactoryPrecomputed>(
        strategy_factory_options);
}

static options::Plugin<MergeStrategyFactory> _plugin_linear(
    "merge_linear", _parse_linear);
}
