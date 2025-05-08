#include "merge_strategy_factory_precomputed.h"

#include "merge_strategy_precomputed.h"
#include "merge_tree_factory.h"
#include "merge_tree.h"

#include "../plugins/plugin.h"

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactoryPrecomputed::MergeStrategyFactoryPrecomputed(
    const shared_ptr<MergeTreeFactory> &merge_tree,
    utils::Verbosity verbosity)
    : MergeStrategyFactory(verbosity),
      merge_tree_factory(merge_tree) {
}

unique_ptr<MergeStrategy> MergeStrategyFactoryPrecomputed::compute_merge_strategy(
    const TaskProxy &task_proxy, const FactoredTransitionSystem &fts) {
    unique_ptr<MergeTree> merge_tree =
        merge_tree_factory->compute_merge_tree(task_proxy);
    return make_unique<MergeStrategyPrecomputed>(fts, move(merge_tree));
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
    if (log.is_at_least_normal()) {
        merge_tree_factory->dump_options(log);
    }
}

class MergeStrategyFactoryPrecomputedFeature
    : public plugins::TypedFeature<MergeStrategyFactory, MergeStrategyFactoryPrecomputed> {
public:
    MergeStrategyFactoryPrecomputedFeature() : TypedFeature("merge_precomputed") {
        document_title("Precomputed merge strategy");
        document_synopsis(
            "This merge strategy has a precomputed merge tree. Note that this "
            "merge strategy does not take into account the current state of "
            "the factored transition system. This also means that this merge "
            "strategy relies on the factored transition system being synchronized "
            "with this merge tree, i.e. all merges are performed exactly as given "
            "by the merge tree.");

        add_option<shared_ptr<MergeTreeFactory>>(
            "merge_tree",
            "The precomputed merge tree.");
        add_merge_strategy_options_to_feature(*this);

        document_note(
            "Note",
            "An example of a precomputed merge startegy is a linear merge strategy, "
            "which can be obtained using:\n"
            "{{{\n"
            "merge_strategy=merge_precomputed(merge_tree=linear(<variable_order>))"
            "\n}}}");
    }
    virtual shared_ptr<MergeStrategyFactoryPrecomputed>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<MergeStrategyFactoryPrecomputed>(
            opts.get<shared_ptr<MergeTreeFactory>>("merge_tree"),
            get_merge_strategy_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<MergeStrategyFactoryPrecomputedFeature> _plugin;
}
