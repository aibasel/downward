#include "por_method.h"

#include "option_parser.h"
#include "plugin.h"

#include <iostream>

using namespace std;

static PluginTypePlugin<PORMethod> _type_plugin(
    "PORMethod",
    "Partial order reduction method");
