#include "lazy_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_lazy {
class LazySearchFeature
    : public plugins::TypedFeature<SearchAlgorithm, lazy_search::LazySearch> {
public:
    LazySearchFeature() : TypedFeature("lazy") {
        document_title("Lazy best-first search");
        document_synopsis("");

        add_option<shared_ptr<OpenListFactory>>("open", "open list");
        add_option<bool>("reopen_closed", "reopen closed nodes", "false");
        add_list_option<shared_ptr<Evaluator>>(
            "preferred",
            "use preferred operators of these evaluators", "[]");
        add_successors_order_options_to_feature(*this);
        add_search_algorithm_options_to_feature(*this, "lazy");
    }

    virtual shared_ptr<lazy_search::LazySearch>
    create_component(const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<lazy_search::LazySearch>(
            opts.get<shared_ptr<OpenListFactory>>("open"),
            opts.get<bool>("reopen_closed"),
            opts.get_list<shared_ptr<Evaluator>>("preferred"),
            get_successors_order_arguments_from_options(opts),
            get_search_algorithm_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<LazySearchFeature> _plugin;
}
