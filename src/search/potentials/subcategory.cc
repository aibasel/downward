#include "../plugins/plugin.h"

namespace potentials {
static class PotentialHeuristicsGroupPlugin : public plugins::SubcategoryPlugin {
public:
    PotentialHeuristicsGroupPlugin() : SubcategoryPlugin("heuristics_potentials") {
        document_title("Potential Heuristics");
    }
}
_subcategory_plugin;
}
