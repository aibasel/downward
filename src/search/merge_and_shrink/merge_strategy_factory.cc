#include "merge_strategy_factory.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactory::MergeStrategyFactory(
    const shared_ptr<AbstractTask> &task, utils::Verbosity verbosity)
    : components::TaskSpecificComponent(task),
      log(utils::get_log_for_verbosity(verbosity)) {
}

void MergeStrategyFactory::dump_options() const {
    if (log.is_at_least_normal()) {
        log << "Merge strategy options:" << endl;
        log << "Type: " << name() << endl;
        dump_strategy_specific_options();
    }
}

void add_merge_strategy_options_to_feature(plugins::Feature &feature) {
    utils::add_log_options_to_feature(feature);
}

tuple<utils::Verbosity> get_merge_strategy_arguments_from_options(
    const plugins::Options &opts) {
    return utils::get_log_arguments_from_options(opts);
}

static class MergeStrategyFactoryCategoryPlugin
    : public plugins::TypedCategoryPlugin<TaskIndependentMergeStrategyFactory> {
public:
    MergeStrategyFactoryCategoryPlugin()
        : TypedCategoryPlugin("MergeStrategy") {
        document_synopsis(
            "This page describes the various merge strategies supported "
            "by the planner. The following paper describes how to improve the "
            "DFP merge strategy with tie-breaking, and presents two new merge "
            "strategies (dyn-MIASM and SCC-DFP):" +
            utils::format_conference_reference(
                {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
                "An Analysis of Merge Strategies for Merge-and-Shrink Heuristics",
                "https://ai.dmi.unibas.ch/papers/sievers-et-al-icaps2016.pdf",
                "Proceedings of the 26th International Conference on Automated "
                "Planning and Scheduling (ICAPS 2016)",
                "294-298", "AAAI Press", "2016"));
    }
} _category_plugin;
}
