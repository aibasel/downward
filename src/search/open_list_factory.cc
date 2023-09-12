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

template<>
unique_ptr<TaskIndependentStateOpenList> TaskIndependentOpenListFactory::create_task_independent_open_list() {
    return create_task_independent_state_open_list();
}

template<>
unique_ptr<TaskIndependentEdgeOpenList> TaskIndependentOpenListFactory::create_task_independent_open_list() {
    return create_task_independent_edge_open_list();
}

static class OpenListFactoryCategoryPlugin : public plugins::TypedCategoryPlugin<TaskIndependentOpenListFactory> {
public:
    OpenListFactoryCategoryPlugin() : TypedCategoryPlugin("TaskIndependentOpenList") {
        // TODO: use document_synopsis() for the wiki page.
    }
}
_category_plugin;
