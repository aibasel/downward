#include "merge_strategy_factory.h"

#include "../plugins/plugin.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactory::MergeStrategyFactory(
        const string &name,
        utils::Verbosity verbosity)
        : name(name),
          log(utils::get_log_for_verbosity(verbosity)) {
}
MergeStrategyFactory::MergeStrategyFactory(const plugins::Options &options)
        : log(utils::get_log_from_options(options)) {
}

void MergeStrategyFactory::dump_options() const {
    if (log.is_at_least_normal()) {
        log << "Merge strategy options:" << endl;
        log << "Type: " << type() << endl;
        dump_strategy_specific_options();
    }
}

void add_merge_strategy_options_to_feature(plugins::Feature &feature, const string &name) {
    utils::add_log_options_to_feature(feature, name);
}

void add_merge_strategy_options_to_feature(plugins::Feature &feature) { // TODO issue1082 remove
    utils::add_log_options_to_feature(feature);
}

static class MergeStrategyFactoryCategoryPlugin : public plugins::TypedCategoryPlugin<MergeStrategyFactory> {
public:
    MergeStrategyFactoryCategoryPlugin() : TypedCategoryPlugin("MergeStrategy") {
        document_synopsis(
            "This page describes the various merge strategies supported "
            "by the planner.");
    }
}
_category_plugin;
}
