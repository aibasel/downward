#include "eager_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

using namespace std;

namespace plugin_eager {
class EagerSearchFeature
    : public plugins::TypedFeature<TaskIndependentSearchAlgorithm> {
public:
    EagerSearchFeature() : TypedFeature("eager") {
        document_title("Eager best-first search");
        document_synopsis(
            "Eager best-first search and its use of preferred operators are "
            "discussed in the following paper:" +
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
        add_option<shared_ptr<TaskIndependentEvaluator>>(
            "f_eval",
            "set evaluator for jump statistics. "
            "(Optional; if no evaluator is used, jump statistics will not be displayed.)",
            plugins::ArgumentInfo::NO_DEFAULT);
        add_list_option<shared_ptr<TaskIndependentEvaluator>>(
            "preferred", "use preferred operators of these evaluators", "[]");
        eager_search::add_eager_search_options_to_feature(*this, "eager");
    }

    virtual shared_ptr<TaskIndependentSearchAlgorithm> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<
            eager_search::EagerSearch, SearchAlgorithm>(
            opts.get<shared_ptr<TaskIndependentOpenListFactory>>("open"),
            opts.get<bool>("reopen_closed"),
            opts.get<shared_ptr<TaskIndependentEvaluator>>("f_eval", nullptr),
            opts.get_list<shared_ptr<TaskIndependentEvaluator>>("preferred"),
            eager_search::get_eager_search_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<EagerSearchFeature> _plugin;
}
