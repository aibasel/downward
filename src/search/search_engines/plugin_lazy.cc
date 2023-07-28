#include "lazy_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_lazy {
class LazySearchFeature : public plugins::TypedFeature<SearchAlgorithm, lazy_search::LazySearch> {
public:
    LazySearchFeature() : TypedFeature("lazy") {
        document_title("Lazy best-first search");
        document_synopsis("");

        add_option<shared_ptr<OpenListFactory>>("open", "open list");
        add_option<bool>("reopen_closed", "reopen closed nodes", "false");
        add_list_option<shared_ptr<Evaluator>>(
            "preferred",
            "use preferred operators of these evaluators", "[]");
        SearchAlgorithm::add_succ_order_options(*this);
        SearchAlgorithm::add_options_to_feature(*this);
    }

    virtual shared_ptr<lazy_search::LazySearch> create_component(const plugins::Options &options, const utils::Context &) const override {
        shared_ptr<lazy_search::LazySearch> search_algorithm = make_shared<lazy_search::LazySearch>(options);
        /*
          TODO: The following two lines look fishy. If they serve a
          purpose, shouldn't the constructor take care of this?
        */
        vector<shared_ptr<Evaluator>> preferred_list = options.get_list<shared_ptr<Evaluator>>("preferred");
        search_algorithm->set_preferred_operator_evaluators(preferred_list);

        return search_algorithm;
    }
};

static plugins::FeaturePlugin<LazySearchFeature> _plugin;
}
