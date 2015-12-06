#include "open_list_factory.h"

#include "open_list.h"

#include "../plugin.h"
#include "../state_id.h"

using namespace std;


template<>
unique_ptr<StateOpenList> OpenListFactory::create_open_list() {
    return create_state_open_list();
}

template<>
unique_ptr<EdgeOpenList> OpenListFactory::create_open_list() {
    return create_edge_open_list();
}


static PluginTypePlugin<OpenListFactory> _type_plugin(
    "OpenList",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
