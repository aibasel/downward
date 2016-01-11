#include "system.h"

#if OPERATING_SYSTEM == LINUX || OPERATING_SYSTEM == OSX
/*
  NOTE:
  Methods with the suffix "_reentrant" are meant to be used in event
  handlers. They should all be "re-entrant", i.e. they must not use
  static variables, global data, or locks. Only some system calls such as
  open/read/write/close are guaranteed to be re-entrant. See
    https://www.securecoding.cert.org/confluence/display/seccode/
    SIG30-C.+Call+only+asynchronous-safe+functions+within+signal+handlers
    #SIG30-C.Callonlyasynchronous-safefunctionswithinsignalhandlers-
    Asynchronous-Signal-SafeFunctions
  for a complete list.
  We also use some low level string methods where re-entrancy is not
  guaranteed but very likely with most compilers. If these ever cause
  any problems, we will have to replace them by re-entrant
  implementations.

  See also: issue479
*/

#include "system_unix.h"

#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <stdlib.h>
#include <unistd.h>

#if OPERATING_SYSTEM == OSX
#include <mach/mach.h>
#endif

using namespace std;

namespace utils {
void write_reentrant(int filedescr, const char *message, int len) {
    while (len > 0) {
        int written;
        do {
            written = write(filedescr, message, len);
        } while (written == -1 && errno == EINTR);
        /*
          We could check for other values of errno here but all errors except
          EINTR are catastrophic enough to abort, so we do not need the
          distinction.
        */
        if (written == -1)
            abort();
        message += written;
        len -= written;
    }
}

void write_reentrant_str(int filedescr, const char *message) {
    write_reentrant(filedescr, message, strlen(message));
}

void write_reentrant_char(int filedescr, char c) {
    write_reentrant(filedescr, &c, 1);
}

void write_reentrant_int(int filedescr, int value) {
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%d", value);
    if (len < 0)
        abort();
    write_reentrant(filedescr, buffer, len);
}

bool read_char_reentrant(int filedescr, char *c) {
    int result;
    do {
        result = read(filedescr, c, 1);
    } while (result == -1 && errno == EINTR);
    /*
      We could check for other values of errno here but all errors except
      EINTR are catastrophic enough to abort, so we do not need the
      distinction.
    */
    if (result == -1)
        abort();
    return result == 1;
}

void print_peak_memory_reentrant() {
#if OPERATING_SYSTEM == OSX
    // TODO: Write print_peak_memory_reentrant() for OS X.
    write_reentrant_str(STDOUT_FILENO, "Peak memory: ");
    write_reentrant_int(STDOUT_FILENO, get_peak_memory_in_kb());
    write_reentrant_str(STDOUT_FILENO, " KB\n");
#else

    int proc_file_descr = TEMP_FAILURE_RETRY(open("/proc/self/status", O_RDONLY));
    if (proc_file_descr == -1) {
        write_reentrant_str(
            STDERR_FILENO,
            "critical error: could not open /proc/self/status\n");
        abort();
    }

    const char magic[] = "\nVmPeak:";
    char c;
    size_t pos_magic = 0;
    const size_t len_magic = sizeof(magic) - 1;

    // Find magic word.
    while (pos_magic != len_magic && read_char_reentrant(proc_file_descr, &c)) {
        if (c == magic[pos_magic]) {
            ++pos_magic;
        } else {
            pos_magic = 0;
        }
    }

    if (pos_magic != len_magic) {
        write_reentrant_str(
            STDERR_FILENO,
            "critical error: could not find VmPeak in /proc/self/status\n");
        abort();
    }

    write_reentrant_str(STDOUT_FILENO, "Peak memory: ");

    // Skip over whitespace.
    while (read_char_reentrant(proc_file_descr, &c) && isspace(c))
        ;

    do {
        write_reentrant_char(STDOUT_FILENO, c);
    } while (read_char_reentrant(proc_file_descr, &c) && !isspace(c));

    write_reentrant_str(STDOUT_FILENO, " KB\n");
    /*
      Ignore potential errors other than EINTR (there is nothing we can do
      about I/O errors or bad file descriptors here).
    */
    TEMP_FAILURE_RETRY(close(proc_file_descr));
#endif
}

#if OPERATING_SYSTEM == LINUX
void exit_handler(int, void *) {
#elif OPERATING_SYSTEM == OSX
void exit_handler() {
#endif
    print_peak_memory_reentrant();
}

void out_of_memory_handler() {
    /*
      We do not use any memory padding currently. The methods below should
      only use stack memory. If we ever run into situations where the stack
      memory is not sufficient, we can consider using sigaltstack to reserve
      memory for the stack of the signal handler and raising a signal here.
    */
    write_reentrant_str(STDOUT_FILENO, "Failed to allocate memory.\n");
    exit_with(ExitCode::OUT_OF_MEMORY);
}

void signal_handler(int signal_number) {
    print_peak_memory_reentrant();
    write_reentrant_str(STDOUT_FILENO, "caught signal ");
    write_reentrant_int(STDOUT_FILENO, signal_number);
    write_reentrant_str(STDOUT_FILENO, " -- exiting\n");
    raise(signal_number);
}

/*
  NOTE: we have two variants of obtaining peak memory information.
        get_peak_memory_in_kb() is used during the regular execution.
        print_peak_memory_in_kb_reentrant() is used in signal handlers.
        The latter is slower but guarantees reentrancy.
*/
int get_peak_memory_in_kb() {
    // On error, produces a warning on cerr and returns -1.
    int memory_in_kb = -1;

#if OPERATING_SYSTEM == OSX
    // Based on http://stackoverflow.com/questions/63166
    task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&t_info),
                  &t_info_count) == KERN_SUCCESS) {
        memory_in_kb = t_info.virtual_size / 1024;
    }
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

void register_event_handlers() {
    // Terminate when running out of memory.
    set_new_handler(out_of_memory_handler);

    // On exit or when receiving certain signals such as SIGINT (Ctrl-C),
    // print the peak memory usage.
#if OPERATING_SYSTEM == LINUX
    on_exit(exit_handler, 0);
#elif OPERATING_SYSTEM == OSX
    atexit(exit_handler);
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

void report_exit_code_reentrant(ExitCode exitcode) {
    const char *message = get_exit_code_message_reentrant(exitcode);
    bool is_error = is_exit_code_error_reentrant(exitcode);
    if (message) {
        int filedescr = is_error ? STDERR_FILENO : STDOUT_FILENO;
        write_reentrant_str(filedescr, message);
        write_reentrant_char(filedescr, '\n');
    } else {
        write_reentrant_str(STDERR_FILENO, "Exitcode: ");
        write_reentrant_int(STDERR_FILENO, static_cast<int>(exitcode));
        write_reentrant_str(STDERR_FILENO, "\nUnknown exitcode.\n");
        abort();
    }
}

int get_process_id() {
    return getpid();
}
}

#endif
