#include "merge_strategy_factory_stateless.h"

#include "merge_selector.h"
#include "merge_strategy_stateless.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/memory.h"

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactoryStateless::MergeStrategyFactoryStateless(
    options::Options &options)
    : merge_selector(options.get<shared_ptr<MergeSelector>>("merge_selector")) {
}

unique_ptr<MergeStrategy> MergeStrategyFactoryStateless::compute_merge_strategy(
    const TaskProxy &task_proxy,
    const FactoredTransitionSystem &fts) {
    merge_selector->initialize(task_proxy);
    return utils::make_unique_ptr<MergeStrategyStateless>(fts, merge_selector);
}

string MergeStrategyFactoryStateless::name() const {
    return "stateless";
}

void MergeStrategyFactoryStateless::dump_strategy_specific_options() const {
    merge_selector->dump_options();
}

bool MergeStrategyFactoryStateless::requires_init_distances() const {
    return merge_selector->requires_init_distances();
}

bool MergeStrategyFactoryStateless::requires_goal_distances() const {
    return merge_selector->requires_goal_distances();
}

static shared_ptr<MergeStrategyFactory>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Stateless merge strategy",
        "This merge strategy has a merge selector, which computes the next "
        "merge only depending on the current state of the factored transition "
        "system, not requiring any additional information.");
    parser.document_note(
        "Note",
        "Examples include the DFP merge strategy, which can be obtained using:\n"
        "{{{\n"
        "merge_strategy=merge_stateless(merge_selector=score_based_filtering("
        "scoring_functions=[goal_relevance,dfp,total_order(<order_option>))]))"
        "\n}}}\n"
        "and the (dynamic/score-based) MIASM strategy, which can be obtained "
        "using:\n"
        "{{{\n"
        "merge_strategy=merge_stateless(merge_selector=score_based_filtering("
        "scoring_functions=[sf_miasm(<shrinking_options>),total_order(<order_option>)]"
        "\n}}}");
    parser.add_option<shared_ptr<MergeSelector>>(
        "merge_selector",
        "The merge selector to be used.");

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeStrategyFactoryStateless>(opts);
}

static options::Plugin<MergeStrategyFactory> _plugin("merge_stateless", _parse);
}
