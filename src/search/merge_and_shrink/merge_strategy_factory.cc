#include "merge_strategy_factory.h"

#include "../options/plugin.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
void MergeStrategyFactory::dump_options() const {
    cout << "Merge strategy options:" << endl;
    cout << "Type: " << name() << endl;
    dump_strategy_specific_options();
}

static options::PluginTypePlugin<MergeStrategyFactory> _type_plugin(
    "MergeStrategy",
    "This page describes the various merge strategies supported "
    "by the planner.");
}
