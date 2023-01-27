#include "eager_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_eager {
class EagerSearchFeature : public plugins::TypedFeature<SearchEngine, eager_search::EagerSearch> {
public:
    EagerSearchFeature() : TypedFeature("eager") {
        document_title("Eager best-first search");
        document_synopsis("");

        add_option<shared_ptr<OpenListFactory>>("open", "open list");
        add_option<bool>(
            "reopen_closed",
            "reopen closed nodes",
            "false");
        add_option<shared_ptr<Evaluator>>(
            "f_eval",
            "set evaluator for jump statistics. "
            "(Optional; if no evaluator is used, jump statistics will not be displayed.)",
            plugins::ArgumentInfo::NO_DEFAULT);
        add_list_option<shared_ptr<Evaluator>>(
            "preferred",
            "use preferred operators of these evaluators",
            "[]");
        eager_search::add_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<EagerSearchFeature> _plugin;
}
