#include "../plugins/plugin.h"

namespace pdbs {
static class PDBGroupPlugin : public plugins::SubcategoryPlugin {
public:
    PDBGroupPlugin() : SubcategoryPlugin("heuristics_pdb") {
        document_title("Pattern database heuristics");
    }
} _subcategory_plugin;
}
