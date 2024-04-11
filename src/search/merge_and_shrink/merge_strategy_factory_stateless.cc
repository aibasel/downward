#include "merge_strategy_factory_stateless.h"

#include "merge_selector.h"
#include "merge_strategy_stateless.h"

#include "../plugins/plugin.h"
#include "../utils/memory.h"

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactoryStateless::MergeStrategyFactoryStateless(
    const shared_ptr<MergeSelector> &merge_selector,
    utils::Verbosity verbosity)
    : MergeStrategyFactory(verbosity),
      merge_selector(merge_selector) {
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
    if (log.is_at_least_normal()) {
        merge_selector->dump_options(log);
    }
}

bool MergeStrategyFactoryStateless::requires_init_distances() const {
    return merge_selector->requires_init_distances();
}

bool MergeStrategyFactoryStateless::requires_goal_distances() const {
    return merge_selector->requires_goal_distances();
}

class MergeStrategyFactoryStatelessFeature
    : public plugins::TypedFeature<MergeStrategyFactory, MergeStrategyFactoryStateless> {
public:
    MergeStrategyFactoryStatelessFeature() : TypedFeature("merge_stateless") {
        document_title("Stateless merge strategy");
        document_synopsis(
            "This merge strategy has a merge selector, which computes the next "
            "merge only depending on the current state of the factored transition "
            "system, not requiring any additional information.");

        add_option<shared_ptr<MergeSelector>>(
            "merge_selector",
            "The merge selector to be used.");
        add_merge_strategy_options_to_feature(*this);

        document_note(
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
    }

    virtual shared_ptr<MergeStrategyFactoryStateless> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<MergeStrategyFactoryStateless>(
            opts.get<shared_ptr<MergeSelector>>("merge_selector"),
            get_merge_strategy_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<MergeStrategyFactoryStatelessFeature> _plugin;
}
