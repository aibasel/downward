#include "eager_search.h"
#include "search_common.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace plugin_eager_wastar {
static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Eager weighted A* search",
        "");
    parser.document_note(
        "Open lists and equivalent statements using general eager search",
        "See corresponding notes for \"(Weighted) A* search (lazy)\"");
    parser.document_note(
        "Note",
        "Eager weighted A* search uses an alternation open list "
        "while A* search uses a tie-breaking open list. Consequently, "
        "\n```\n--search eager_wastar([h()], w=1)\n```\n"
        "is **not** equivalent to\n```\n--search astar(h())\n```\n");

    parser.add_list_option<shared_ptr<Evaluator>>(
        "evals",
        "evaluators");
    parser.add_list_option<shared_ptr<Evaluator>>(
        "preferred",
        "use preferred operators of these evaluators",
        "[]");
    parser.add_option<bool>(
        "reopen_closed",
        "reopen closed nodes",
        "true");
    parser.add_option<int>(
        "boost",
        "boost value for preferred operator open lists",
        "0");
    parser.add_option<int>(
        "w",
        "evaluator weight",
        "1");

    eager_search::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run()) {
        return nullptr;
    } else {
        opts.set("open", search_common::create_wastar_open_list_factory(opts));
        return make_shared<eager_search::EagerSearch>(opts);
    }
}

static Plugin<SearchEngine> _plugin("eager_wastar", _parse);
}
