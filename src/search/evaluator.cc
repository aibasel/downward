#include "evaluator.h"

#include "plugin.h"

using namespace std;


bool Evaluator::dead_ends_are_reliable() const {
    return true;
}

bool Evaluator::reevaluate_and_check_if_changed(EvaluationContext &) {
    return false;
}

static PluginTypePlugin<Evaluator> _type_plugin(
    "Evaluator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
