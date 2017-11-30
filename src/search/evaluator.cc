#include "evaluator.h"

#include "plugin.h"

using namespace std;


bool Evaluator::dead_ends_are_reliable() const {
    return true;
}


static PluginTypePlugin<Evaluator> _type_plugin(
    "Evaluator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
