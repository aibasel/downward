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


shared_ptr<OpenListFactory> TaskIndependentOpenListFactory::create_task_specific_OpenListFactory(const shared_ptr<AbstractTask> &task, int depth) {
    utils::g_log << std::string(depth, ' ') << "Creating OpenListFactory as root component..." << endl;
    std::unique_ptr<ComponentMap> component_map = std::make_unique<ComponentMap>();
    return create_task_specific_OpenListFactory(task, component_map, depth);
}


shared_ptr<OpenListFactory> TaskIndependentOpenListFactory::create_task_specific_OpenListFactory(
        [[maybe_unused]] const shared_ptr<AbstractTask> &task,
        [[maybe_unused]] unique_ptr<ComponentMap> &component_map,
        [[maybe_unused]] int depth) {
    cerr << "Tries to create OpenListFactory in an unimplemented way." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}


shared_ptr<Component> TaskIndependentOpenListFactory::create_task_specific_Component(const shared_ptr<AbstractTask> &task, unique_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<OpenListFactory> x = create_task_specific_OpenListFactory(task, component_map, depth);
    return static_pointer_cast<Component>(x);
}


static class OpenListFactoryCategoryPlugin : public plugins::TypedCategoryPlugin<TaskIndependentOpenListFactory> {
public:
    OpenListFactoryCategoryPlugin() : TypedCategoryPlugin("OpenList") {
        // TODO: use document_synopsis() for the wiki page.
    }
}
_category_plugin;
