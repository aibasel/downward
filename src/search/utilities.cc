#include "utilities.h"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <limits>
#include <sstream>
#include <unistd.h>
using namespace std;

#ifdef __APPLE__
#include <mach/mach.h>
#endif

#ifdef __APPLE__
static void exit_handler();
#else
static void exit_handler(int exit_code, void *hint);
#endif

static void signal_handler(int signal_number);

void register_event_handlers() {
    // On exit or when receiving certain signals such as SIGINT (Ctrl-C),
    // print the peak memory usage.
#ifdef __APPLE__
    atexit(exit_handler);
#else
    on_exit(exit_handler, 0);
#endif
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGINT, signal_handler);
}

#ifdef __APPLE__
void exit_handler() {
#else
void exit_handler(int, void *) {
#endif
    print_peak_memory();
}

void signal_handler(int signal_number) {
    // See glibc manual: "Handlers That Terminate the Process"
    static volatile sig_atomic_t handler_in_progress = 0;
    if (handler_in_progress)
        raise(signal_number);
    handler_in_progress = 1;
    print_peak_memory();
    cout << "caught signal " << signal_number << " -- exiting" << endl;
    signal(signal_number, SIG_DFL);
    raise(signal_number);
}

int get_peak_memory_in_kb() {
    // On error, produces a warning on cerr and returns -1.
    int memory_in_kb = -1;

#ifdef __APPLE__
    // Based on http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
    task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&t_info),
                  &t_info_count) == KERN_SUCCESS)
        memory_in_kb = t_info.virtual_size / 1024;
#else
    ostringstream filename_stream;
    filename_stream << "/proc/" << getpid() << "/status";
    const char *filename = filename_stream.str().c_str();

    ifstream procfile(filename);
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
