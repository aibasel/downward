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



shared_ptr<AbstractTask> TaskIndependentAbstractTask::create_task_specific_AbstractTask(const shared_ptr<AbstractTask> &task, int depth) {
    utils::g_log << "Creating AbstractTask as root component..." << endl;
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_AbstractTask(task, component_map, depth);
}

shared_ptr<AbstractTask> TaskIndependentAbstractTask::create_task_specific_AbstractTask([[maybe_unused]] const shared_ptr<AbstractTask> &task, [[maybe_unused]] shared_ptr<ComponentMap> &component_map, int depth) {
    cerr << "Tries to create AbstractTask in an unimplemented way." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}


shared_ptr<Component> TaskIndependentAbstractTask::create_task_specific_Component(const shared_ptr<AbstractTask> &task, shared_ptr<ComponentMap> &component_map, int depth) {
    shared_ptr<AbstractTask> x = create_task_specific_AbstractTask(task, component_map, depth);
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
