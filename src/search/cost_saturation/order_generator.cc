#include "order_generator.h"

#include "../plugin.h"

using namespace std;

namespace cost_saturation {
static PluginTypePlugin<OrderGenerator> _type_plugin(
    "OrderGenerator",
    "Generate heuristic orders.");
}
