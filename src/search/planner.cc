#include "option_parser.h"
#include "search_engine.h"

#include "utils/system.h"
#include "utils/timer.h"

#include <iostream>

using namespace std;
using utils::ExitCode;

int main(int argc, const char **argv) {
    utils::register_event_handlers();

    if (argc < 2) {
        cout << OptionParser::usage(argv[0]) << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }

    if (static_cast<string>(argv[1]) != "--help")
        read_everything(cin);

    shared_ptr<SearchEngine> engine;

    // The command line is parsed twice: once in dry-run mode, to
    // check for simple input errors, and then in normal mode.
    bool unit_cost = is_unit_cost();
    try {
        OptionParser::parse_cmd_line(argc, argv, true, unit_cost);
        engine = OptionParser::parse_cmd_line(argc, argv, false, unit_cost);
    } catch (ArgError &error) {
        cerr << error << endl;
        OptionParser::usage(argv[0]);
        utils::exit_with(ExitCode::INPUT_ERROR);
    } catch (ParseError &error) {
        cerr << error << endl;
        utils::exit_with(ExitCode::INPUT_ERROR);
    }

    utils::Timer search_timer;
    engine->search();
    search_timer.stop();
    utils::g_timer.stop();

    engine->save_plan_if_necessary();
    engine->print_statistics();
    cout << "Search time: " << search_timer << endl;
    cout << "Total time: " << utils::g_timer << endl;

    if (engine->found_solution()) {
        utils::exit_with(ExitCode::PLAN_FOUND);
    } else {
        utils::exit_with(ExitCode::UNSOLVED_INCOMPLETE);
    }
}
