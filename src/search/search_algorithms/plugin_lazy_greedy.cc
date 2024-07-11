#include "lazy_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_lazy_greedy {
static const string DEFAULT_LAZY_BOOST = "1000";

class LazyGreedySearchFeature
    : public plugins::TypedFeature<SearchAlgorithm, lazy_search::LazySearch> {
public:
    LazyGreedySearchFeature() : TypedFeature("lazy_greedy") {
        document_title("Greedy search (lazy)");
        document_synopsis("");

        add_list_option<shared_ptr<Evaluator>>(
            "evals",
            "evaluators");
        add_option<int>(
            "boost",
            "boost value for alternation queues that are restricted "
            "to preferred operator nodes",
            DEFAULT_LAZY_BOOST);

        add_option<bool>(
            "reopen_closed",
            "reopen closed nodes",
            "false");
        add_list_option<shared_ptr<Evaluator>>(
            "preferred",
            "use preferred operators of these evaluators",
            "[]");
        add_successors_order_options_to_feature(*this);
        add_search_algorithm_options_to_feature(*this, "lazy_greedy");

        document_note(
            "Open lists",
            "In most cases, lazy greedy best first search uses "
            "an alternation open list with one queue for each evaluator. "
            "If preferred operator evaluators are used, it adds an "
            "extra queue for each of these evaluators that includes "
            "only the nodes that are generated with a preferred operator. "
            "If only one evaluator and no preferred operator evaluator is used, "
            "the search does not use an alternation open list "
            "but a standard open list with only one queue.");
        document_note(
            "Equivalent statements using general lazy search",
            "\n```\n--evaluator h2=eval2\n"
            "--search lazy_greedy([eval1, h2], preferred=[h2], boost=100)\n```\n"
            "is equivalent to\n"
            "```\n--evaluator h1=eval1 --heuristic h2=eval2\n"
            "--search lazy(alt([single(h1), single(h1, pref_only=true), single(h2),\n"
            "                  single(h2, pref_only=true)], boost=100),\n"
            "              preferred=[h2])\n```\n"
            "------------------------------------------------------------\n"
            "```\n--search lazy_greedy([eval1, eval2], boost=100)\n```\n"
            "is equivalent to\n"
            "```\n--search lazy(alt([single(eval1), single(eval2)], boost=100))\n```\n"
            "------------------------------------------------------------\n"
            "```\n--evaluator h1=eval1\n--search lazy_greedy([h1], preferred=[h1])\n```\n"
            "is equivalent to\n"
            "```\n--evaluator h1=eval1\n"
            "--search lazy(alt([single(h1), single(h1, pref_only=true)], boost=1000),\n"
            "              preferred=[h1])\n```\n"
            "------------------------------------------------------------\n"
            "```\n--search lazy_greedy([eval1])\n```\n"
            "is equivalent to\n"
            "```\n--search lazy(single(eval1))\n```\n",
            true);
    }

    virtual shared_ptr<lazy_search::LazySearch> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<lazy_search::LazySearch>(
            search_common::create_greedy_open_list_factory(
                opts.get_list<shared_ptr<Evaluator>>("evals"),
                opts.get_list<shared_ptr<Evaluator>>("preferred"),
                opts.get<int>("boost")
                ),
            opts.get<bool>("reopen_closed"),
            opts.get_list<shared_ptr<Evaluator>>("preferred"),
            get_successors_order_arguments_from_options(opts),
            get_search_algorithm_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<LazyGreedySearchFeature> _plugin;
}
