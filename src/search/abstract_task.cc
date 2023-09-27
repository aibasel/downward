#include "abstract_task.h"

#include "per_task_information.h"

#include "plugins/plugin.h"

#include <iostream>

using namespace std;

const FactPair FactPair::no_fact = FactPair(-1, -1);

ostream &operator<<(ostream &os, const FactPair &fact_pair) {
    os << fact_pair.var << "=" << fact_pair.value;
    return os;
}


TaskIndependentAbstractTask::TaskIndependentAbstractTask() {
}



shared_ptr<AbstractTask> TaskIndependentAbstractTask::create_task_specific_AbstractTask(shared_ptr<AbstractTask> &task) {
    utils::g_log << "Creating AbstractTask as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_AbstractTask(task, component_map);
}

shared_ptr<AbstractTask> TaskIndependentAbstractTask::create_task_specific_AbstractTask([[maybe_unused]] shared_ptr<AbstractTask> &task, [[maybe_unused]] shared_ptr<ComponentMap> &component_map) {
    cerr << "Tries to create AbstractTask in an unimplemented way." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}


shared_ptr<Component> TaskIndependentAbstractTask::create_task_specific_Component(shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map) {
    shared_ptr<AbstractTask> x = create_task_specific_AbstractTask(task, component_map);
    return static_pointer_cast<Component>(x);
}


static class TaskIndependentAbstractTaskCategoryPlugin : public plugins::TypedCategoryPlugin<TaskIndependentAbstractTask> {
public:
    TaskIndependentAbstractTaskCategoryPlugin() : TypedCategoryPlugin("TI_AbstractTask") {
        // TODO: Replace empty string by synopsis for the wiki page.
        document_synopsis("");
    }
}
_category_plugin;
