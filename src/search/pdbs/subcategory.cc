#include "../plugins/plugin.h"

namespace pdbs {
static class PDBGroupPlugin : public plugins::SubcategoryPlugin {
public:
    PDBGroupPlugin() : SubcategoryPlugin("heuristics_pdb") {
        document_title("Pattern Database Heuristics");
    }
}
_subcategory_plugin;
}
