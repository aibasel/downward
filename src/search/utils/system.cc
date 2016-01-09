#include "system.h"

namespace utils {
const char *get_exit_code_message_reentrant(ExitCode exitcode) {
    switch (exitcode) {
    case ExitCode::PLAN_FOUND:
        return "Solution found.";
    case ExitCode::CRITICAL_ERROR:
        return "Unexplained error occurred.";
    case ExitCode::INPUT_ERROR:
        return "Usage error occurred.";
    case ExitCode::UNSUPPORTED:
        return "Tried to use unsupported feature.";
    case ExitCode::UNSOLVABLE:
        return "Task is provably unsolvable.";
    case ExitCode::UNSOLVED_INCOMPLETE:
        return "Search stopped without finding a solution.";
    case ExitCode::OUT_OF_MEMORY:
        return "Memory limit has been reached.";
    default:
        return nullptr;
    }
}

bool is_exit_code_error_reentrant(ExitCode exitcode) {
    switch (exitcode) {
    case ExitCode::PLAN_FOUND:
    case ExitCode::UNSOLVABLE:
    case ExitCode::UNSOLVED_INCOMPLETE:
    case ExitCode::OUT_OF_MEMORY:
        return false;
    case ExitCode::CRITICAL_ERROR:
    case ExitCode::INPUT_ERROR:
    case ExitCode::UNSUPPORTED:
    default:
        return true;
    }
}

void exit_with(ExitCode exitcode) {
    report_exit_code_reentrant(exitcode);
    exit(static_cast<int>(exitcode));
}
}
