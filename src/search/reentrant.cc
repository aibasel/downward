// For linux, we have an OS-specific implementation in reentrant_linux.cc
#if OPERATING_SYSTEM != LINUX
// TODO: find re-entrant alternative on other systems.
#include "reentrant.h"

#include "utilities.h"

#include <csignal>
#include <iostream>
using namespace std;


void print_peak_memory() {
    cout << "Peak memory: "
         << get_peak_memory_in_kb() << " KB" << endl;
}

#if OPERATING_SYSTEM == OSX
void exit_handler() {
    print_peak_memory();
}
#endif

void out_of_memory_handler() {
    cout << "Failed to allocate memory." << endl;
    exit_with(EXIT_OUT_OF_MEMORY);
}

void signal_handler(int signal_number) {
    print_peak_memory();
    cout << "caught signal " << signal_number
         << " -- exiting" << endl;
    raise(signal_number);
}

void report_exit_code_reentrant(int exitcode) {
    const char *message;
    bool is_error = false;
    switch (exitcode) {
    case EXIT_PLAN_FOUND:
        message = "Solution found.";
        break;
    case EXIT_CRITICAL_ERROR:
        message = "Unexplained error occurred.";
        is_error = true;
        break;
    case EXIT_INPUT_ERROR:
        message = "Usage error occurred.";
        is_error = true;
        break;
    case EXIT_UNSUPPORTED:
        message = "Tried to use unsupported feature.";
        is_error = true;
        break;
    case EXIT_UNSOLVABLE:
        message = "Task is provably unsolvable.";
        break;
    case EXIT_UNSOLVED_INCOMPLETE:
        message = "Search stopped without finding a solution.";
        break;
    case EXIT_OUT_OF_MEMORY:
        message = "Memory limit has been reached.";
        break;
    case EXIT_TIMEOUT:
        message = "Time limit has been reached.";
        break;
    default:
        cerr << "Exitcode: " << exitcode << endl
             << "Unknown exitcode." << endl;
        abort();
    }
    ostream &stream = is_error ? cerr : cout;
    stream << message << endl;
}
#endif
