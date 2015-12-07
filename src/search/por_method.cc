#include "por_method.h"

#include "plugin.h"

using namespace std;

static PluginTypePlugin<PORMethod> _type_plugin(
    "PORMethod",
    "Partial order reduction method");
