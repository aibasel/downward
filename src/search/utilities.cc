#include "utilities.h"

#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <unistd.h>
using namespace std;


#if OPERATING_SYSTEM == LINUX
static void exit_handler(int exit_code, void *hint);
#elif OPERATING_SYSTEM == OSX
static void exit_handler();
#include <mach/mach.h>
#elif OPERATING_SYSTEM == WINDOWS || OPERATING_SYSTEM == CYGWIN
#include <windows.h>
#include <psapi.h>
#endif

static void out_of_memory_handler();
extern "C" void signal_handler(int signal_number);


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

void write_reentrant(int filedescr, const char *message) {
    int len = strlen(message);
    if (write(filedescr, message, len) != len) {
        abort();
    }
}

void write_reentrant_char(int filedescr, const char c) {
    if (write(filedescr, &c, 1) != 1) {
        abort();
    }
}

void print_peak_memory_reentrant() {
#if OPERATING_SYSTEM == OSX || OPERATING_SYSTEM == WINDOWS || OPERATING_SYSTEM == CYGWIN
    print_peak_memory();
#else
    // TODO: Test for EINTR
    int proc_file_descr = open("/proc/self/status", O_RDONLY);

    if (proc_file_descr == -1) {
        write_reentrant(STDERR_FILENO,
                        "critical error: could not open /proc/self/status\n");
        abort();
    }

    const char magic[] = "\nVmPeak:";
    char c;
    size_t pos_magic = 0;
    const size_t len_magic = sizeof(magic) - 1;

    // Find magic word.
    while (pos_magic != len_magic && read(proc_file_descr, &c, 1) > 0) {
        if (c == magic[pos_magic]) {
            ++pos_magic;
        } else {
            pos_magic = 0;
        }
    }

    if (pos_magic != len_magic) {
        write_reentrant(STDERR_FILENO,
                        "critical error: could not find VmPeak in /proc/self/status\n");
        abort();
    }

    write_reentrant(STDOUT_FILENO, "Peak memory: ");

    // Skip over whitespace.
    while (read(proc_file_descr, &c, 1) > 0 && isspace(c))
        ;

    do {
        write_reentrant_char(STDOUT_FILENO, c);
    } while (read(proc_file_descr, &c, 1) > 0 && !isspace(c));

    write_reentrant(STDOUT_FILENO, " KB\n");
    close(proc_file_descr);
#endif
}

#if OPERATING_SYSTEM == LINUX || OPERATING_SYSTEM == OSX
#if OPERATING_SYSTEM == LINUX
void exit_handler(int, void *) {
#elif OPERATING_SYSTEM == OSX
void exit_handler() {
#endif
    print_peak_memory_reentrant();
}
#endif

void exit_with(ExitCode exitcode) {
    switch (exitcode) {
    case EXIT_PLAN_FOUND:
        write_reentrant(STDOUT_FILENO, "Solution found.\n");
        break;
    case EXIT_CRITICAL_ERROR:
        write_reentrant(STDERR_FILENO, "Unexplained error occurred.\n");
        break;
    case EXIT_INPUT_ERROR:
        write_reentrant(STDERR_FILENO, "Usage error occurred.\n");
        break;
    case EXIT_UNSUPPORTED:
        write_reentrant(STDERR_FILENO, "Tried to use unsupported feature.\n");
        break;
    case EXIT_UNSOLVABLE:
        write_reentrant(STDOUT_FILENO, "Task is provably unsolvable.\n");
        break;
    case EXIT_UNSOLVED_INCOMPLETE:
        write_reentrant(STDOUT_FILENO, "Search stopped without finding a solution.\n");
        break;
    case EXIT_OUT_OF_MEMORY:
        write_reentrant(STDOUT_FILENO, "Memory limit has been reached.\n");
        break;
    case EXIT_TIMEOUT:
        write_reentrant(STDOUT_FILENO, "Time limit has been reached.\n");
        break;
    default:
        char buffer[32];
        if (sprintf(buffer, "Exitcode: %d\n", exitcode) < 0)
            abort();
        write_reentrant(STDERR_FILENO, buffer);
        ABORT("Unknown exitcode.");
    }
    exit(exitcode);
}

static void out_of_memory_handler() {
    write_reentrant(STDOUT_FILENO, "Failed to allocate memory.\n");
    exit_with(EXIT_OUT_OF_MEMORY);
}

void signal_handler(int signal_number) {
    print_peak_memory_reentrant();
    char buffer[32];
    if (sprintf(buffer, "caught signal %d -- exiting\n", signal_number) < 0)
        abort();
    write_reentrant(STDOUT_FILENO, buffer);
    raise(signal_number);
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
