#include "merge_strategy_factory.h"

#include "../options/plugin.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactory::MergeStrategyFactory(const options::Options &options)
    : log(utils::get_log_from_options(options)) {
}

void MergeStrategyFactory::dump_options() const {
    log << "Merge strategy options:" << endl;
    log << "Type: " << name() << endl;
    dump_strategy_specific_options();
}

void add_merge_strategy_options_to_parser(options::OptionParser &parser) {
    utils::add_log_options_to_parser(parser);
}

static options::PluginTypePlugin<MergeStrategyFactory> _type_plugin(
    "MergeStrategy",
    "This page describes the various merge strategies supported "
    "by the planner.");
}
