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

        add_option<shared_ptr<TaskIndependentOpenListFactory>>("open", "open list");
        add_option<bool>("reopen_closed", "reopen closed nodes", "false");
        add_list_option<shared_ptr<TaskIndependentEvaluator>>(
            "preferred", "use preferred operators of these evaluators", "[]");
        add_successors_order_options_to_feature(*this);
        add_search_algorithm_options_to_feature(*this, "lazy");
    }

    virtual shared_ptr<lazy_search::LazySearch> create_component(
        const plugins::Options &opts) const override {
        Cache cache; // issue559 remove

        return plugins::make_shared_from_arg_tuples<lazy_search::LazySearch>(
            tasks::g_root_task, opts.get<shared_ptr<TaskIndependentOpenListFactory>>("open")->bind_task(tasks::g_root_task),
            opts.get<bool>("reopen_closed"),
            bind_task_recursively(opts.get_list<shared_ptr<TaskIndependentEvaluator>>("preferred"), tasks::g_root_task, cache),
            get_successors_order_arguments_from_options(opts),
            get_search_algorithm_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LazySearchFeature> _plugin;
}
