#include "open_list_factory.h"

#include "plugins/plugin.h"

using namespace std;

template<>
shared_ptr<StateOpenList> OpenListFactory::create_open_list() {
    return create_state_open_list();
}

template<>
shared_ptr<EdgeOpenList> OpenListFactory::create_open_list() {
    return create_edge_open_list();
}


shared_ptr<OpenListFactory> TaskIndependentOpenListFactory::create_task_specific_OpenListFactory(shared_ptr<AbstractTask> &task) {
    utils::g_log << "Creating OpenListFactory as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_OpenListFactory(task, component_map);
}


shared_ptr<OpenListFactory> TaskIndependentOpenListFactory::create_task_specific_OpenListFactory([[maybe_unused]] shared_ptr<AbstractTask> &task, [[maybe_unused]] shared_ptr<ComponentMap> &component_map) {
    cerr << "Tries to create OpenListFactory in an unimplemented way." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}


shared_ptr<Component> TaskIndependentOpenListFactory::create_task_specific_Component(shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map) {
    shared_ptr<OpenListFactory> x = create_task_specific_OpenListFactory(task, component_map);
    return static_pointer_cast<Component>(x);
}


static class OpenListFactoryCategoryPlugin : public plugins::TypedCategoryPlugin<TaskIndependentOpenListFactory> {
public:
    OpenListFactoryCategoryPlugin() : TypedCategoryPlugin("OpenList") {
        // TODO: use document_synopsis() for the wiki page.
    }
}
_category_plugin;
