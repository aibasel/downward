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
    : MergeStrategyFactory(),
      merge_tree_factory(options.get<shared_ptr<MergeTreeFactory>>("merge_tree")) {
}

unique_ptr<MergeStrategy> MergeStrategyFactoryPrecomputed::compute_merge_strategy(
    shared_ptr<AbstractTask> task,
    FactoredTransitionSystem &fts) {
    unique_ptr<MergeTree> merge_tree =
        merge_tree_factory->compute_merge_tree(task, fts);
//    merge_tree->inorder_traversal(4);
    return utils::make_unique_ptr<MergeStrategyPrecomputed>(fts, move(merge_tree));
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
        "the merge strategy does not take into account the current state of "
        "the FTS. That also means that this merge strategy relies on the FTS"
        "being obtained by strictly following this merge strategy.");
    parser.add_option<shared_ptr<MergeTreeFactory>>(
        "merge_tree",
        "the precomputed merge tree");

    options::Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeStrategyFactoryPrecomputed>(opts);
}

static options::PluginShared<MergeStrategyFactory> _plugin("merge_precomputed", _parse);
}
