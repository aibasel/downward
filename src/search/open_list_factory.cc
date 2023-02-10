#include "open_list_factory.h"

#include "plugins/plugin.h"

using namespace std;


template<>
unique_ptr<StateOpenList> OpenListFactory::create_open_list() {
    return create_state_open_list();
}

template<>
unique_ptr<EdgeOpenList> OpenListFactory::create_open_list() {
    return create_edge_open_list();
}

static class OpenListFactoryCategoryPlugin : public plugins::TypedCategoryPlugin<OpenListFactory> {
public:
    OpenListFactoryCategoryPlugin() : TypedCategoryPlugin("OpenList") {
        // TODO: use document_synopsis() for the wiki page.
    }
}
_category_plugin;
