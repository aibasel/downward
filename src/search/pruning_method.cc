#include "por_method.h"

#include "plugin.h"

using namespace std;

static PluginTypePlugin<PORMethod> _type_plugin(
    "PORMethod",
    "Prune applicable operators based on partial order reduction.");
