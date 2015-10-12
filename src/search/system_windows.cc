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

#if OPERATING_SYSTEM == CYGWIN
#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID (clockid_t(2))
#endif
#endif
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
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc), sizeof(pmc));
    if (memory_in_kb == -1) {
        cerr << "warning: could not determine peak memory" << endl;
        return -1;
    }
    return pmc.PeakPagefileUsage / 1024;
}

void register_event_handlers() {
    // When running out of memory, release some emergency memory and
    // terminate.
    set_new_handler(out_of_memory_handler);

    struct sigaction default_signal_action;
    default_signal_action.sa_handler = signal_handler;
    // Block all signals we handle while one of them is handled.
    sigemptyset(&default_signal_action.sa_mask);
    sigaddset(&default_signal_action.sa_mask, SIGABRT);
    sigaddset(&default_signal_action.sa_mask, SIGTERM);
    sigaddset(&default_signal_action.sa_mask, SIGSEGV);
    sigaddset(&default_signal_action.sa_mask, SIGINT);
    sigaddset(&default_signal_action.sa_mask, SIGXCPU);
    // Reset handler to default action after completion.
    default_signal_action.sa_flags = SA_RESETHAND;

    sigaction(SIGABRT, &default_signal_action, 0);
    sigaction(SIGTERM, &default_signal_action, 0);
    sigaction(SIGSEGV, &default_signal_action, 0);
    sigaction(SIGINT, &default_signal_action, 0);
    sigaction(SIGXCPU, &default_signal_action, 0);
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

int get_process_id() {
    return _getpid();
}
#endif
