#include "eager_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_eager {
class TaskIndependentEagerSearchFeature : public plugins::TypedFeature<TaskIndependentSearchAlgorithm, eager_search::TaskIndependentEagerSearch> {
public:
    TaskIndependentEagerSearchFeature() : TypedFeature("eager") {
        document_title("Eager best-first search");
        document_synopsis("");

        add_option<shared_ptr<TaskIndependentOpenListFactory>>("open", "open list");
        add_option<bool>(
            "reopen_closed",
            "reopen closed nodes",
            "false");
        add_option<shared_ptr<TaskIndependentEvaluator>>(
            "f_eval",
            "set evaluator for jump statistics. "
            "(Optional; if no evaluator is used, jump statistics will not be displayed.)",
            plugins::ArgumentInfo::NO_DEFAULT);
        add_list_option<shared_ptr<TaskIndependentEvaluator>>(
            "preferred",
            "use preferred operators of these evaluators",
            "[]");
        eager_search::add_options_to_feature(*this, "eager");
    }

    virtual shared_ptr<eager_search::TaskIndependentEagerSearch> create_component(const plugins::Options &opts, const utils::Context &) const override {
        return make_shared<eager_search::TaskIndependentEagerSearch>(opts.get<string>("name"),
                                                                     opts.get<utils::Verbosity>("verbosity"),
                                                                     opts.get<OperatorCost>("cost_type"),
                                                                     opts.get<double>("max_time"),
                                                                     opts.get<int>("bound"),
                                                                     opts.get<bool>("reopen_closed"),
                                                                     opts.get<shared_ptr<TaskIndependentOpenListFactory>>("open"),
                                                                     opts.get_list<shared_ptr<TaskIndependentEvaluator>>("preferred"),
                                                                     opts.get<shared_ptr<PruningMethod>>("pruning"),
                                                                     opts.get<shared_ptr<TaskIndependentEvaluator>>("f_eval", nullptr),
                                                                     opts.get<shared_ptr<TaskIndependentEvaluator>>("lazy_evaluator", nullptr),
                                                                     opts.get_unparsed_config());
    }
};

static plugins::FeaturePlugin<TaskIndependentEagerSearchFeature> _plugin;
}
