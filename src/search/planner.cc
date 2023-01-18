#include "command_line.h"
#include "search_engine.h"

#include "plugins/errors.h"
#include "plugins/registries.h"
#include "tasks/root_task.h"
#include "task_utils/task_properties.h"
#include "utils/logging.h"
#include "utils/system.h"
#include "utils/timer.h"

#include <iostream>

using namespace std;
using utils::ExitCode;

int main(int argc, const char **argv) {
    utils::register_event_handlers();

    g_program_name = argv[0];

    if (argc < 2) {
        utils::g_log << usage() << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }

    bool unit_cost = false;
    if (static_cast<string>(argv[1]) != "--help") {
        utils::g_log << "reading input..." << endl;
        tasks::read_root_task(cin);
        utils::g_log << "done reading input!" << endl;
        TaskProxy task_proxy(*tasks::g_root_task);
        unit_cost = task_properties::is_unit_cost(task_proxy);
    }

    shared_ptr<SearchEngine> engine;

    // The command line is parsed twice: once in dry-run mode, to
    // check for simple input errors, and then in normal mode.
    try {
        plugins::Registry registry(*plugins::RawRegistry::instance());
        parse_cmd_line(argc, argv, registry, true, unit_cost);
        engine = parse_cmd_line(argc, argv, registry, false, unit_cost);
    } catch (const ArgError &error) {
        error.print();
        usage();
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    } catch (const plugins::OptionParserError &error) {
        error.print();
        usage();
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    } catch (const plugins::ParseError &error) {
        error.print();
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }

    utils::Timer search_timer;
    engine->search();
    search_timer.stop();
    utils::g_timer.stop();

    engine->save_plan_if_necessary();
    engine->print_statistics();
    utils::g_log << "Search time: " << search_timer << endl;
    utils::g_log << "Total time: " << utils::g_timer << endl;

    ExitCode exitcode = engine->found_solution()
        ? ExitCode::SUCCESS
        : ExitCode::SEARCH_UNSOLVED_INCOMPLETE;
    utils::report_exit_code_reentrant(exitcode);
    return static_cast<int>(exitcode);
}
