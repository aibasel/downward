#include "search_engine.h"
#include "task_independent_search_engine.h"


#include "utils/system.h"

using namespace std;
using utils::ExitCode;

shared_ptr <SearchEngine> TaskIndependentSearchEngine::create_task_specific(shared_ptr <AbstractTask> &_task) {
    //std::shared_ptr<AbstractTask> task = tasks::g_root_task;
    return make_shared<SearchEngine>(verbosity,
                                     cost_type,
                                     max_time,
                                     bound,
                                     description,
                                     _task);
}

TaskIndependentSearchEngine::TaskIndependentSearchEngine(utils::Verbosity verbosity,
                           OperatorCost cost_type,
                           double max_time,
                           int bound,
                           string unparsed_config)
        : description(unparsed_config),
          status(IN_PROGRESS),
          solution_found(false),
          verbosity(verbosity),
          bound(bound),
          cost_type(cost_type),
          max_time(max_time) {
    if (bound < 0) {
        cerr << "error: negative cost bound " << bound << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

TaskIndependentSearchEngine::~TaskIndependentSearchEngine() {
}

static class TaskIndependentSearchEngineCategoryPlugin : public plugins::TypedCategoryPlugin<TaskIndependentSearchEngine> {
public:
    TaskIndependentSearchEngineCategoryPlugin() : TypedCategoryPlugin("TaskIndependentSearchEngine") {
        // TODO: Replace add synopsis for the wiki page.
        // document_synopsis("...");
    }
}_category_plugin;