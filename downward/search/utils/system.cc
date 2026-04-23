#include "system.h"

#include <cstdlib>

using namespace std;

namespace utils {
const char *get_exit_code_message_reentrant(ExitCode exitcode) {
    switch (exitcode) {
    case ExitCode::SUCCESS:
        return "Solution found.";
    case ExitCode::SEARCH_CRITICAL_ERROR:
        return "Unexplained error occurred.";
    case ExitCode::SEARCH_INPUT_ERROR:
        return "Usage error occurred.";
    case ExitCode::SEARCH_UNSUPPORTED:
        return "Tried to use unsupported feature.";
    case ExitCode::SEARCH_UNSOLVABLE:
        return "Task is provably unsolvable.";
    case ExitCode::SEARCH_UNSOLVED_INCOMPLETE:
        return "Search stopped without finding a solution.";
    case ExitCode::SEARCH_OUT_OF_MEMORY:
        return "Memory limit has been reached.";
    case ExitCode::SEARCH_OUT_OF_TIME:
        return "Time limit has been reached.";
    default:
        return nullptr;
    }
}

bool is_exit_code_error_reentrant(ExitCode exitcode) {
    switch (exitcode) {
    case ExitCode::SUCCESS:
    case ExitCode::SEARCH_UNSOLVABLE:
    case ExitCode::SEARCH_UNSOLVED_INCOMPLETE:
    case ExitCode::SEARCH_OUT_OF_MEMORY:
    case ExitCode::SEARCH_OUT_OF_TIME:
        return false;
    case ExitCode::SEARCH_CRITICAL_ERROR:
    case ExitCode::SEARCH_INPUT_ERROR:
    case ExitCode::SEARCH_UNSUPPORTED:
    default:
        return true;
    }
}

void report_exit_code(ExitCode exitcode) {
    const char *message = get_exit_code_message_reentrant(exitcode);
    bool is_error = is_exit_code_error_reentrant(exitcode);
    if (message) {
        ostream &stream = is_error ? cerr : cout;
        stream << message << endl;
    } else {
        cerr << "Exitcode: " << static_cast<int>(exitcode) << endl
             << "Unknown exitcode." << endl;
        abort();
    }
}

void exit_with(ExitCode exitcode) {
    report_exit_code(exitcode);
    throw ExitException(exitcode);
}

void exit_with_reentrant(ExitCode exitcode) {
    /* In signal handlers or when we run out of memory, we have to use the
       "safe function" _Exit() rather than the unsafe function exit(). */
    report_exit_code_reentrant(exitcode);
    _Exit(static_cast<int>(exitcode));
}
}
