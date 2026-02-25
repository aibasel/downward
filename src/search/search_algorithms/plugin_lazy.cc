#include "lazy_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_lazy {
class LazySearchFeature
    : public plugins::TaskIndependentFeature<TaskIndependentSearchAlgorithm> {
public:
    LazySearchFeature() : TaskIndependentFeature("lazy") {
        document_title("Lazy best-first search");
        document_synopsis("");

        add_option<shared_ptr<TaskIndependentOpenListFactory>>("open", "open list");
        add_option<bool>("reopen_closed", "reopen closed nodes", "false");
        add_list_option<shared_ptr<TaskIndependentEvaluator>>(
            "preferred", "use preferred operators of these evaluators", "[]");
        add_successors_order_options_to_feature(*this);
        add_search_algorithm_options_to_feature(*this, "lazy");
    }

    virtual shared_ptr<TaskIndependentSearchAlgorithm> create_component(
        const plugins::Options &opts) const override {
        return components::make_shared_component<lazy_search::LazySearch, SearchAlgorithm>(
            opts.get<shared_ptr<TaskIndependentOpenListFactory>>("open"),
            opts.get<bool>("reopen_closed"),
            opts.get_list<shared_ptr<TaskIndependentEvaluator>>("preferred"),
            get_successors_order_arguments_from_options(opts),
            get_search_algorithm_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LazySearchFeature> _plugin;
}
