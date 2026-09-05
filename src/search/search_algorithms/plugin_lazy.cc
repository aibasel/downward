#include "lazy_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

using namespace std;

namespace plugin_lazy {
class LazySearchFeature
    : public plugins::TypedFeature<TaskIndependentSearchAlgorithm> {
public:
    LazySearchFeature() : TypedFeature("lazy") {
        document_title("Lazy best-first search");
        document_synopsis(
            "Deferred evaluation and preferred operators are described in "
            "the following paper:" +
            utils::format_conference_reference(
                {"Silvia Richter", "Malte Helmert"},
                "Preferred Operators and Deferred Evaluation in "
                "Satisficing Planning",
                "https://ojs.aaai.org/index.php/ICAPS/article/download/13345/13193",
                "Proceedings of the 19th International Conference on "
                "Automated Planning and Scheduling (ICAPS 2009)",
                "273-280", "AAAI Press", "2009"));

        add_option<shared_ptr<TaskIndependentOpenListFactory>>(
            "open", "open list");
        add_option<bool>("reopen_closed", "reopen closed nodes", "false");
        add_list_option<shared_ptr<TaskIndependentEvaluator>>(
            "preferred", "use preferred operators of these evaluators", "[]");
        add_successors_order_options_to_feature(*this);
        add_search_algorithm_options_to_feature(*this, "lazy");
    }

    virtual shared_ptr<TaskIndependentSearchAlgorithm> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<
            lazy_search::LazySearch, SearchAlgorithm>(
            opts.get<shared_ptr<TaskIndependentOpenListFactory>>("open"),
            opts.get<bool>("reopen_closed"),
            opts.get_list<shared_ptr<TaskIndependentEvaluator>>("preferred"),
            get_successors_order_arguments_from_options(opts),
            get_search_algorithm_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LazySearchFeature> _plugin;
}
