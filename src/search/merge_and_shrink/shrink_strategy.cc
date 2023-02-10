#include "shrink_strategy.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../plugins/plugin.h"

#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace merge_and_shrink {
void ShrinkStrategy::dump_options(utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        log << "Shrink strategy options: " << endl;
        log << "Type: " << name() << endl;
        dump_strategy_specific_options(log);
    }
}

string ShrinkStrategy::get_name() const {
    return name();
}

static class ShrinkStrategyCategoryPlugin : public plugins::TypedCategoryPlugin<ShrinkStrategy> {
public:
    ShrinkStrategyCategoryPlugin() : TypedCategoryPlugin("ShrinkStrategy") {
        document_synopsis(
            "This page describes the various shrink strategies supported "
            "by the planner."
            );
    }
}
_category_plugin;
}
