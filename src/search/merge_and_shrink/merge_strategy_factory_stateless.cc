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
    : MergeStrategyFactory(),
      merge_selector(options.get<shared_ptr<MergeSelector>>("merge_selector")) {
}

unique_ptr<MergeStrategy> MergeStrategyFactoryStateless::compute_merge_strategy(
    shared_ptr<AbstractTask> task,
    FactoredTransitionSystem &fts) {
    merge_selector->initialize(task);
    return utils::make_unique_ptr<MergeStrategyStateless>(fts, merge_selector);
}

string MergeStrategyFactoryStateless::name() const {
    return "stateless";
}

void MergeStrategyFactoryStateless::dump_strategy_specific_options() const {
    merge_selector->dump_options();
}

static shared_ptr<MergeStrategyFactory>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Stateless merge strategy",
        "This type of merge strategies computes the next merge only depending "
        "on the current state of the factored transition system, not storing "
        "any additional information.");
    parser.add_option<shared_ptr<MergeSelector>>(
        "merge_selector",
        "The merge selector to be used.");

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeStrategyFactoryStateless>(opts);
}

static options::PluginShared<MergeStrategyFactory> _plugin("merge_stateless", _parse);
}
