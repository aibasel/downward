#include "lazy_search.h"
#include "search_common.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace plugin_lazy {
static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
    parser.document_synopsis("Lazy best-first search", "");
    parser.add_option<shared_ptr<OpenListFactory>>("open", "open list");
    parser.add_option<bool>("reopen_closed", "reopen closed nodes", "false");
    parser.add_list_option<Heuristic *>(
        "preferred",
        "use preferred operators of these heuristics", "[]");
    SearchEngine::add_succ_order_options(parser);
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();

    shared_ptr<lazy_search::LazySearch> engine;
    if (!parser.dry_run()) {
        engine = make_shared<lazy_search::LazySearch>(opts);
        /*
          TODO: The following two lines look fishy. If they serve a
          purpose, shouldn't the constructor take care of this?
        */
        vector<Heuristic *> preferred_list = opts.get_list<Heuristic *>("preferred");
        engine->set_pref_operator_heuristics(preferred_list);
    }

    return engine;
}
static PluginShared<SearchEngine> _plugin("lazy", _parse);
}
