#include "../plugins/plugin.h"

namespace evaluators_plugin_group {
static class EvaluatorGroupPlugin : public plugins::SubcategoryPlugin {
public:
    EvaluatorGroupPlugin() : SubcategoryPlugin("evaluators_basic") {
        document_title("Basic evaluators");
    }
} _subcategory_plugin;
}
