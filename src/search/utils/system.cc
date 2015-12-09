#include "system.h"

#include "utilities.h"


const char *get_exit_code_message_reentrant(int exitcode) {
    switch (exitcode) {
    case EXIT_PLAN_FOUND:
        return "Solution found.";
    case EXIT_CRITICAL_ERROR:
        return "Unexplained error occurred.";
    case EXIT_INPUT_ERROR:
        return "Usage error occurred.";
    case EXIT_UNSUPPORTED:
        return "Tried to use unsupported feature.";
    case EXIT_UNSOLVABLE:
        return "Task is provably unsolvable.";
    case EXIT_UNSOLVED_INCOMPLETE:
        return "Search stopped without finding a solution.";
    case EXIT_OUT_OF_MEMORY:
        return "Memory limit has been reached.";
    default:
        return nullptr;
    }
}

bool is_exit_code_error_reentrant(int exitcode) {
    switch (exitcode) {
    case EXIT_PLAN_FOUND:
    case EXIT_UNSOLVABLE:
    case EXIT_UNSOLVED_INCOMPLETE:
    case EXIT_OUT_OF_MEMORY:
        return false;
    case EXIT_CRITICAL_ERROR:
    case EXIT_INPUT_ERROR:
    case EXIT_UNSUPPORTED:
    default:
        return true;
    }
}
