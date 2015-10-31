#include "scalar_evaluator.h"

#include "plugin.h"

using namespace std;


bool ScalarEvaluator::dead_ends_are_reliable() const {
    return true;
}


static PluginTypePlugin<ScalarEvaluator> _type_plugin(
    "ScalarEvaluator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
