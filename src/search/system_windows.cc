#include "system.h"

#if OPERATING_SYSTEM == WINDOWS || OPERATING_SYSTEM == CYGWIN
// TODO: find re-entrant alternatives on Windows/Cygwin.

#include "system_windows.h"
#include "utilities.h"

#include <csignal>
#include <ctime>
#include <iostream>
#include <process.h>
#include <psapi.h>

using namespace std;


void out_of_memory_handler() {
    cout << "Failed to allocate memory." << endl;
    exit_with(EXIT_OUT_OF_MEMORY);
}

void signal_handler(int signal_number) {
    cout << "Peak memory: "
         << get_peak_memory_in_kb() << " KB" << endl;
    cout << "caught signal " << signal_number
         << " -- exiting" << endl;
    raise(signal_number);
}

int get_peak_memory_in_kb() {
    // The file /proc/self/status is present under Cygwin, but contains no peak memory info.
    PROCESS_MEMORY_COUNTERS_EX pmc;
    bool success = GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc),
        sizeof(pmc));
    if (!success) {
        cerr << "warning: could not determine peak memory" << endl;
        return -1;
    }
    return pmc.PeakPagefileUsage / 1024;
}

void register_event_handlers() {
    // Terminate when running out of memory.
    set_new_handler(out_of_memory_handler);

    // On Windows, sigaction() is not available, so we use the deprecated
    // alternative signal(). The main difference is that signal() does not
    // block other signals while the signal handler is running. This can lead
    // to race conditions and undefined behaviour. For details, see
    // http://stackoverflow.com/questions/231912/what-is-the-difference-between-sigaction-and-signal
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGINT, signal_handler);
    // SIGXCPU is not supported on Windows.
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
    default:
        cerr << "Exitcode: " << exitcode << endl
             << "Unknown exitcode." << endl;
        abort();
    }
    ostream &stream = is_error ? cerr : cout;
    stream << message << endl;
}

int get_process_id() {
    return _getpid();
}
#endif
