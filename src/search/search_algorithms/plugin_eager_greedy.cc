#include "eager_search.h"
#include "search_common.h"

#include "../plugins/plugin.h"

using namespace std;

namespace plugin_eager_greedy {
class EagerGreedySearchFeature
    : public plugins::TypedFeature<SearchAlgorithm, eager_search::EagerSearch> {
public:
    EagerGreedySearchFeature() : TypedFeature("eager_greedy") {
        document_title("Greedy search (eager)");
        document_synopsis("");

        add_list_option<shared_ptr<Evaluator>>("evals", "evaluators");
        add_list_option<shared_ptr<Evaluator>>(
            "preferred",
            "use preferred operators of these evaluators", "[]");
        add_option<int>(
            "boost",
            "boost value for preferred operator open lists", "0");
        eager_search::add_eager_search_options_to_feature(
            *this, "eager_greedy");

        document_note(
            "Open list",
            "In most cases, eager greedy best first search uses "
            "an alternation open list with one queue for each evaluator. "
            "If preferred operator evaluators are used, it adds an extra queue "
            "for each of these evaluators that includes only the nodes that "
            "are generated with a preferred operator. "
            "If only one evaluator and no preferred operator evaluator is used, "
            "the search does not use an alternation open list but a "
            "standard open list with only one queue.");
        document_note(
            "Closed nodes",
            "Closed node are not re-opened");
        document_note(
            "Equivalent statements using general eager search",
            "\n```\n--evaluator h2=eval2\n"
            "--search eager_greedy([eval1, h2], preferred=[h2], boost=100)\n```\n"
            "is equivalent to\n"
            "```\n--evaluator h1=eval1 --heuristic h2=eval2\n"
            "--search eager(alt([single(h1), single(h1, pref_only=true), single(h2), \n"
            "                    single(h2, pref_only=true)], boost=100),\n"
            "               preferred=[h2])\n```\n"
            "------------------------------------------------------------\n"
            "```\n--search eager_greedy([eval1, eval2])\n```\n"
            "is equivalent to\n"
            "```\n--search eager(alt([single(eval1), single(eval2)]))\n```\n"
            "------------------------------------------------------------\n"
            "```\n--evaluator h1=eval1\n"
            "--search eager_greedy([h1], preferred=[h1])\n```\n"
            "is equivalent to\n"
            "```\n--evaluator h1=eval1\n"
            "--search eager(alt([single(h1), single(h1, pref_only=true)]),\n"
            "               preferred=[h1])\n```\n"
            "------------------------------------------------------------\n"
            "```\n--search eager_greedy([eval1])\n```\n"
            "is equivalent to\n"
            "```\n--search eager(single(eval1))\n```\n", true);
    }

    virtual shared_ptr<eager_search::EagerSearch> create_component(
        const plugins::Options &opts,
        const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<Evaluator>>(
            context, opts, "evals");

        return plugins::make_shared_from_arg_tuples<eager_search::EagerSearch>(
            search_common::create_greedy_open_list_factory(
                opts.get_list<shared_ptr<Evaluator>>("evals"),
                opts.get_list<shared_ptr<Evaluator>>("preferred"),
                opts.get<int>("boost")
                ),
            false,
            nullptr,
            opts.get_list<shared_ptr<Evaluator>>("preferred"),
            eager_search::get_eager_search_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<EagerGreedySearchFeature> _plugin;
}
