#include "command_line.h"
#include "git_revision.h"
#include "search_algorithm.h"

#include "task_utils/task_properties.h"
#include "tasks/root_task.h"
#include "utils/logging.h"
#include "utils/system.h"
#include "utils/timer.h"

#include <iostream>

using namespace std;
using utils::ExitCode;

int main(int argc, const char **argv) {
    try {
        if (argc == 2 &&
            static_cast<string>(argv[1]) == "--internal-git-revision") {
            // We handle this option before registering event handlers to avoid
            // printing peak memory on exit.
            cout << g_git_revision << endl;
            exit(0);
        }
        utils::register_event_handlers();

        if (argc < 2) {
            utils::g_log << get_revision_info() << endl;
            utils::g_log << get_usage(argv[0]) << endl;
            utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
        }

        shared_ptr<AbstractTask> task;
        bool unit_cost = false;
        if (static_cast<string>(argv[1]) != "--help") {
            utils::g_log << get_revision_info() << endl;
            utils::g_log << "reading input..." << endl;
            tasks::read_root_task(cin);
            task = tasks::g_root_task;
            /*
              TODO once we get rid of g_root_task, the two lines above should
              store be replaced by something like
                  task = tasks::read_task(cin)
            */
            utils::g_log << "done reading input!" << endl;
            TaskProxy task_proxy(*task);
            unit_cost = task_properties::is_unit_cost(task_proxy);
        }

        ParsedSearchOptions parsed_search_options =
            parse_cmd_line(argc, argv, unit_cost);

        shared_ptr<SearchAlgorithm> search_algorithm;
        if (parsed_search_options.search_algorithm) {
            search_algorithm =
                parsed_search_options.search_algorithm->bind_task(task);
            PlanManager &plan_manager = search_algorithm->get_plan_manager();
            plan_manager.set_plan_filename(parsed_search_options.plan_filename);
            plan_manager.set_num_previously_generated_plans(
                parsed_search_options.num_previously_generated_plans);
            plan_manager.set_is_part_of_anytime_portfolio(
                parsed_search_options.is_part_of_anytime_portfolio);
        }

        utils::Timer search_timer;
        search_algorithm->search();
        search_timer.stop();
        utils::g_timer.stop();

        search_algorithm->save_plan_if_necessary();
        search_algorithm->print_statistics();
        utils::g_log << "Search time: " << search_timer << endl;
        utils::g_log << "Total time: " << utils::g_timer << endl;

        ExitCode exitcode = search_algorithm->found_solution()
                                ? ExitCode::SUCCESS
                                : ExitCode::SEARCH_UNSOLVED_INCOMPLETE;
        exit_with(exitcode);
    } catch (const utils::ExitException &e) {
        /* To ensure that all destructors are called before the program exits,
           we raise an exception in utils::exit_with() and let main() return. */
        return static_cast<int>(e.get_exitcode());
    }
}
