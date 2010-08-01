#include <csignal>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <limits>
#include <sstream>
using namespace std;

#include "utilities.h"

static void exit_handler(int exit_code, void *hint);
static void signal_handler(int signal_number);

void register_event_handlers() {
    // On exit or when receiving certain signals such as SIGINT (Ctrl-C),
    // print the peak memory usage.
    on_exit(exit_handler, 0);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGINT, signal_handler);
}

void exit_handler(int, void *) {
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
    ostringstream filename_stream;
    filename_stream << "/proc/" << getpid() << "/status";
    const char *filename = filename_stream.str().c_str();

    ifstream procfile(filename);
    string word;
    while (procfile.good()) {
        procfile >> word;
        if (word == "VmPeak:") {
            int memory_kb;
            procfile >> memory_kb;
            if (procfile.fail())
                break;
            return memory_kb;
        }
        // Skip to end of line.
        procfile.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cerr << "warning: error reading memory from procfile" << endl;
    return -1;
}

void print_peak_memory() {
    cout << "Peak memory: " << get_peak_memory_in_kb() << " KB" << endl;
}
