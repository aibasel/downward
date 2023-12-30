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


TaskIndependentAbstractTask::TaskIndependentAbstractTask() : name("abstract_task"), log(utils::g_log) {
}


shared_ptr<AbstractTask> TaskIndependentAbstractTask::get_task_specific(
    [[maybe_unused]] const shared_ptr<AbstractTask> &task,
    [[maybe_unused]] unique_ptr<ComponentMap> &component_map,
    [[maybe_unused]] int depth) const {
    cerr << "Tries to create AbstractTask in an unimplemented way." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}

static class TaskIndependentAbstractTaskCategoryPlugin : public plugins::TypedCategoryPlugin<TaskIndependentAbstractTask> {
public:
    TaskIndependentAbstractTaskCategoryPlugin() : TypedCategoryPlugin("AbstractTask") {
        // TODO: Replace empty string by synopsis for the wiki page.
        document_synopsis("");
    }
}
_category_plugin;
