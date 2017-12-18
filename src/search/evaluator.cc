#include "evaluator.h"

#include "plugin.h"

using namespace std;


bool Evaluator::dead_ends_are_reliable() const {
    return true;
}

std::pair<bool, bool> Evaluator::reevaluate_and_check_if_changed(EvaluationContext &) {
    return std::pair<bool,bool>(false,false);
}

static PluginTypePlugin<Evaluator> _type_plugin(
    "Evaluator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
