#include "utilities.h"

#include "reentrant.h"

#include <cassert>
#include <csignal>
#include <fstream>
#include <iostream>
#include <limits>
using namespace std;

void register_event_handlers() {
    // When running out of memory, release some emergency memory and
    // terminate.
    set_new_handler(out_of_memory_handler);

    // On exit or when receiving certain signals such as SIGINT (Ctrl-C),
    // print the peak memory usage.
#if OPERATING_SYSTEM == LINUX
    on_exit(exit_handler, 0);
#elif OPERATING_SYSTEM == OSX
    atexit(exit_handler);
#elif OPERATING_SYSTEM == CYGWIN || OPERATING_SYSTEM == WINDOWS
    // nothing
#endif
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

void exit_with(ExitCode exitcode) {
    report_exit_code_reentrant(exitcode);
    exit(exitcode);
}

int get_peak_memory_in_kb() {
    // On error, produces a warning on cerr and returns -1.
    int memory_in_kb = -1;

#if OPERATING_SYSTEM == OSX
    // Based on http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
    task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&t_info),
                  &t_info_count) == KERN_SUCCESS)
        memory_in_kb = t_info.virtual_size / 1024;
#elif OPERATING_SYSTEM == WINDOWS || OPERATING_SYSTEM == CYGWIN
    // The file /proc/self/status is present under Cygwin, but contains no peak memory info.
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc), sizeof(pmc));
    memory_in_kb = pmc.PeakPagefileUsage / 1024;
#else
    ifstream procfile;
    procfile.open("/proc/self/status");
    string word;
    while (procfile.good()) {
        procfile >> word;
        if (word == "VmPeak:") {
            procfile >> memory_in_kb;
            break;
        }
        // Skip to end of line.
        procfile.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    if (procfile.fail())
        memory_in_kb = -1;
#endif

    if (memory_in_kb == -1)
        cerr << "warning: could not determine peak memory" << endl;
    return memory_in_kb;
}

void print_peak_memory() {
    cout << "Peak memory: " << get_peak_memory_in_kb() << " KB" << endl;
}


bool is_product_within_limit(int factor1, int factor2, int limit) {
    assert(factor1 >= 0 && factor1 <= limit);
    assert(factor2 >= 0 && factor2 <= limit);
    return factor2 == 0 || factor1 <= limit / factor2;
}
