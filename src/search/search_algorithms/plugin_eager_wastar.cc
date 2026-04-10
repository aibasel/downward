#include "eager_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_eager_wastar {
class EagerWAstarSearchFeature
    : public plugins::TaskIndependentFeature<TaskIndependentSearchAlgorithm> {
public:
    EagerWAstarSearchFeature() : TaskIndependentFeature("eager_wastar") {
        document_title("Eager weighted A* search");
        document_synopsis("");

        add_list_option<shared_ptr<TaskIndependentEvaluator>>(
            "evals", "evaluators");
        add_list_option<shared_ptr<TaskIndependentEvaluator>>(
            "preferred", "use preferred operators of these evaluators", "[]");
        add_option<bool>("reopen_closed", "reopen closed nodes", "true");
        add_option<int>(
            "boost", "boost value for preferred operator open lists", "0");
        add_option<int>("w", "evaluator weight", "1");
        eager_search::add_eager_search_options_to_feature(
            *this, "eager_wastar");

        document_note(
            "Open lists and equivalent statements using general eager search",
            "See corresponding notes for \"(Weighted) A* search (lazy)\"");
        document_note(
            "Note",
            "Eager weighted A* search uses an alternation open list "
            "while A* search uses a tie-breaking open list. Consequently, "
            "\n```\n--search \"eager_wastar([h()], w=1)\"\n```\n"
            "is **not** equivalent to\n```\n--search \"astar(h())\"\n```\n");
    }

    virtual shared_ptr<TaskIndependentSearchAlgorithm> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<
            eager_search::EagerSearch, SearchAlgorithm>(
            search_common::create_wastar_open_list_factory(
                opts.get_list<shared_ptr<TaskIndependentEvaluator>>("evals"),
                opts.get_list<shared_ptr<TaskIndependentEvaluator>>(
                    "preferred"),
                opts.get<int>("boost"), opts.get<int>("w"),
                opts.get<utils::Verbosity>("verbosity")),
            opts.get<bool>("reopen_closed"),
            opts.get<shared_ptr<TaskIndependentEvaluator>>("f_eval", nullptr),
            opts.get_list<shared_ptr<TaskIndependentEvaluator>>("preferred"),
            eager_search::get_eager_search_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<EagerWAstarSearchFeature> _plugin;
}
