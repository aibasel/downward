#include "utilities.h"

#include <cassert>
#include <csignal>
#include <fstream>
#include <limits>

using namespace std;

#if OPERATING_SYSTEM == LINUX
#include <unistd.h>
static void exit_handler(int exit_code, void *hint);
#elif OPERATING_SYSTEM == OSX
#include <unistd.h>
static void exit_handler();
#include <mach/mach.h>
#elif OPERATING_SYSTEM == WINDOWS || OPERATING_SYSTEM == CYGWIN
#include "utilities_windows.h"
#include <process.h>
#include <psapi.h>
#endif

// See issue 469 for the reasons we chose this limit.
static char *memory_padding = new char[32 * 1024];

static void out_of_memory_handler();
static void signal_handler(int signal_number);


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
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGINT, signal_handler);
#if OPERATING_SYSTEM != WINDOWS
    // This causes problems, see issue479.
    //signal(SIGXCPU, signal_handler);
#endif
}

#if OPERATING_SYSTEM == LINUX || OPERATING_SYSTEM == OSX
#if OPERATING_SYSTEM == LINUX
void exit_handler(int, void *) {
#elif OPERATING_SYSTEM == OSX
void exit_handler() {
#endif
    print_peak_memory(false);
}
#endif

void exit_with(ExitCode exitcode) {
    switch (exitcode) {
    case EXIT_PLAN_FOUND:
        cout << "Solution found." << endl;
        break;
    case EXIT_CRITICAL_ERROR:
        cerr << "Unexplained error occurred." << endl;
        break;
    case EXIT_INPUT_ERROR:
        cerr << "Usage error occurred." << endl;
        break;
    case EXIT_UNSUPPORTED:
        cerr << "Tried to use unsupported feature." << endl;
        break;
    case EXIT_UNSOLVABLE:
        cout << "Task is provably unsolvable." << endl;
        break;
    case EXIT_UNSOLVED_INCOMPLETE:
        cout << "Search stopped without finding a solution." << endl;
        break;
    case EXIT_OUT_OF_MEMORY:
        cout << "Memory limit has been reached." << endl;
        break;
    default:
        cerr << "Exitcode: " << exitcode << endl;
        ABORT("Unknown exitcode.");
    }
    exit(exitcode);
}

static void out_of_memory_handler() {
    assert(memory_padding);
    delete[] memory_padding;
    memory_padding = 0;
    cout << "Failed to allocate memory. Released memory buffer." << endl;
    exit_with(EXIT_OUT_OF_MEMORY);
}

void signal_handler(int signal_number) {
    // See glibc manual: "Handlers That Terminate the Process"
    static volatile sig_atomic_t handler_in_progress = 0;
    if (handler_in_progress)
        raise(signal_number);
    handler_in_progress = 1;
    print_peak_memory(false);
    cout << "caught signal " << signal_number << " -- exiting" << endl;
    signal(signal_number, SIG_DFL);
    raise(signal_number);
}

int get_peak_memory_in_kb(bool use_buffered_input) {
    // On error, produces a warning on cerr and returns -1.
    int memory_in_kb = -1;

#if OPERATING_SYSTEM == OSX
    unused_parameter(use_buffered_input);
    // Based on http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
    task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&t_info),
                  &t_info_count) == KERN_SUCCESS)
        memory_in_kb = t_info.virtual_size / 1024;
#elif OPERATING_SYSTEM == WINDOWS || OPERATING_SYSTEM == CYGWIN
    unused_parameter(use_buffered_input);
    // The file /proc/self/status is present under Cygwin, but contains no peak memory info.
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc), sizeof(pmc));
    memory_in_kb = pmc.PeakPagefileUsage / 1024;
#else
    ifstream procfile;
    if (!use_buffered_input) {
        procfile.rdbuf()->pubsetbuf(0, 0);
    }
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

void print_peak_memory(bool use_buffered_input) {
    cout << "Peak memory: " << get_peak_memory_in_kb(use_buffered_input) << " KB" << endl;
}


bool is_product_within_limit(int factor1, int factor2, int limit) {
    assert(factor1 >= 0);
    assert(factor2 >= 0);
    assert(limit >= 0);
    return factor2 == 0 || factor1 <= limit / factor2;
}


int get_process_id() {
#if OPERATING_SYSTEM == WINDOWS
    return _getpid();
#else
    return getpid();
#endif
}
