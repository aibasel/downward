#include "eager_search.h"
#include "search_common.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace plugin_eager_wastar {
static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
    parser.document_synopsis("Weighted eager best-first search", "");

    parser.add_list_option<shared_ptr<Evaluator>>(
        "evals",
        "evaluators");
    parser.add_list_option<shared_ptr<Evaluator>>(
        "preferred",
        "use preferred operators of these evaluators", "[]");
    parser.add_option<bool>(
        "reopen_closed",
        "reopen closed nodes", "true");
    parser.add_option<int>(
        "boost",
        "boost value for preferred operator open lists", "0");
    parser.add_option<int>(
        "w",
        "evaluator weight", "1");

    SearchEngine::add_pruning_option(parser);
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    shared_ptr<eager_search::EagerSearch> engine;
    if (!parser.dry_run()) {
        opts.set("open", search_common::create_wastar_open_list_factory(opts));
        engine = make_shared<eager_search::EagerSearch>(opts);
    }

    return engine;
}

static Plugin<SearchEngine> _plugin("eager_wastar", _parse);
}
