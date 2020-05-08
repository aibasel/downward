#include "system_windows.h"

#if OPERATING_SYSTEM == WINDOWS

// TODO: find re-entrant alternatives on Windows.

#include <csignal>
#include <ctime>
#include <iostream>
#include <process.h>
#include <psapi.h>

using namespace std;

namespace utils {
void out_of_memory_handler() {
    cout << "Failed to allocate memory." << endl;
    exit_with(ExitCode::SEARCH_OUT_OF_MEMORY);
}

void signal_handler(int signal_number) {
    cout << "Peak memory: "
         << get_peak_memory_in_kb() << " KB" << endl;
    cout << "caught signal " << signal_number
         << " -- exiting" << endl;
    raise(signal_number);
}

int get_peak_memory_in_kb() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    bool success = GetProcessMemoryInfo(
        GetCurrentProcess(),
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

    /*
      On Windows, sigaction() is not available, so we use the deprecated
      alternative signal(). The main difference is that signal() does not block
      other signals while the signal handler is running. This can lead to race
      conditions and undefined behaviour. For details, see
      http://stackoverflow.com/questions/231912
    */
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGINT, signal_handler);
    // SIGXCPU is not supported on Windows.
}

void report_exit_code_reentrant(ExitCode exitcode) {
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

int get_process_id() {
    return _getpid();
}
}

#endif
