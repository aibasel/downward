#include "shrink_strategy.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../plugin.h"

#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
void ShrinkStrategy::dump_options() const {
    utils::g_log << "Shrink strategy options: " << endl;
    utils::g_log << "Type: " << name() << endl;
    dump_strategy_specific_options();
}

string ShrinkStrategy::get_name() const {
    return name();
}

static PluginTypePlugin<ShrinkStrategy> _type_plugin(
    "ShrinkStrategy",
    "This page describes the various shrink strategies supported "
    "by the planner.");
}
