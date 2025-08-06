#include "../plugins/plugin.h"

namespace potentials {
static class PotentialHeuristicsGroupPlugin
    : public plugins::SubcategoryPlugin {
public:
    PotentialHeuristicsGroupPlugin()
        : SubcategoryPlugin("heuristics_potentials") {
        document_title("Potential heuristics");
    }
} _subcategory_plugin;
}
