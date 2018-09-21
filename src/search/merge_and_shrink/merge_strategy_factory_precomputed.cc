#include "merge_strategy_factory_precomputed.h"

#include "merge_strategy_precomputed.h"
#include "merge_tree_factory.h"
#include "merge_tree.h"

#include "../options/option_parser.h"
#include "../options/options.h"
#include "../options/plugin.h"

#include "../utils/memory.h"

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactoryPrecomputed::MergeStrategyFactoryPrecomputed(
    options::Options &options)
    : merge_tree_factory(options.get<shared_ptr<MergeTreeFactory>>("merge_tree")) {
}

unique_ptr<MergeStrategy> MergeStrategyFactoryPrecomputed::compute_merge_strategy(
    const TaskProxy &task_proxy,
    const FactoredTransitionSystem &fts) {
    unique_ptr<MergeTree> merge_tree =
        merge_tree_factory->compute_merge_tree(task_proxy);
    return utils::make_unique_ptr<MergeStrategyPrecomputed>(fts, move(merge_tree));
}

bool MergeStrategyFactoryPrecomputed::requires_init_distances() const {
    return merge_tree_factory->requires_init_distances();
}

bool MergeStrategyFactoryPrecomputed::requires_goal_distances() const {
    return merge_tree_factory->requires_goal_distances();
}

string MergeStrategyFactoryPrecomputed::name() const {
    return "precomputed";
}

void MergeStrategyFactoryPrecomputed::dump_strategy_specific_options() const {
    merge_tree_factory->dump_options();
}

static shared_ptr<MergeStrategyFactory>_parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Precomputed merge strategy",
        "This merge strategy has a precomputed merge tree. Note that this "
        "merge strategy does not take into account the current state of "
        "the factored transition system. This also means that this merge "
        "strategy relies on the factored transition system being synchronized "
        "with this merge tree, i.e. all merges are performed exactly as given "
        "by the merge tree.");
    parser.add_option<shared_ptr<MergeTreeFactory>>(
        "merge_tree",
        "The precomputed merge tree");

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeStrategyFactoryPrecomputed>(opts);
}

static options::Plugin<MergeStrategyFactory> _plugin("merge_precomputed", _parse);
}
