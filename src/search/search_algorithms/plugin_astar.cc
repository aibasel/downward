#include "eager_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_astar {
class AStarSearchFeature
    : public plugins::TypedFeature<SearchAlgorithm, eager_search::EagerSearch> {
public:
    AStarSearchFeature() : TypedFeature("astar") {
        document_title("A* search (eager)");
        document_synopsis(
            "A* is a special case of eager best first search that uses g+h "
            "as f-function. "
            "We break ties using the evaluator. Closed nodes are re-opened.");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator for h-value");
        add_option<shared_ptr<Evaluator>>(
            "lazy_evaluator",
            "An evaluator that re-evaluates a state before it is expanded.",
            plugins::ArgumentInfo::NO_DEFAULT);
        eager_search::add_eager_search_options_to_feature(
            *this, "astar");

        document_note(
            "lazy_evaluator",
            "When a state s is taken out of the open list, the lazy evaluator h "
            "re-evaluates s. If h(s) changes (for example because h is path-dependent), "
            "s is not expanded, but instead reinserted into the open list. "
            "This option is currently only present for the A* algorithm.");
        document_note(
            "Equivalent statements using general eager search",
            "\n```\n--search astar(evaluator)\n```\n"
            "is equivalent to\n"
            "```\n--evaluator h=evaluator\n"
            "--search eager(tiebreaking([sum([g(), h]), h], unsafe_pruning=false),\n"
            "               reopen_closed=true, f_eval=sum([g(), h]))\n"
            "```\n", true);
    }

    virtual shared_ptr<eager_search::EagerSearch> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        plugins::Options options_copy(opts);
        auto temp =
            search_common::create_astar_open_list_factory_and_f_eval(
                opts.get<shared_ptr<Evaluator>>("eval"),
                opts.get<utils::Verbosity>("verbosity"));
        options_copy.set("open", temp.first);
        options_copy.set("f_eval", temp.second);
        options_copy.set("reopen_closed", true);
        vector<shared_ptr<Evaluator>> preferred_list;
        options_copy.set("preferred", preferred_list);
        return plugins::make_shared_from_arg_tuples<eager_search::EagerSearch>(
            options_copy.get<shared_ptr<OpenListFactory>>("open"),
            options_copy.get<bool>("reopen_closed"),
            options_copy.get<shared_ptr<Evaluator>>("f_eval", nullptr),
            options_copy.get_list<shared_ptr<Evaluator>>("preferred"),
            eager_search::get_eager_search_arguments_from_options(
                options_copy)
            );
    }
};

static plugins::FeaturePlugin<AStarSearchFeature> _plugin;
}
