#include "evaluator.h"

#include "plugin.h"

using namespace std;


Evaluator::Evaluator(const string &description)
    : description(description){

}

bool Evaluator::dead_ends_are_reliable() const {
    return true;
}

string Evaluator::get_description() const {
    return description;
}


static PluginTypePlugin<Evaluator> _type_plugin(
    "Evaluator",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
