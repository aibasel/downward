#include "reentrant.h"

#include "utilities.h"

#include <csignal>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

void write_reentrant(int filedescr, const char *message, int len) {
    while(len > 0) {
        int written = TEMP_FAILURE_RETRY(write(filedescr, message, len));
        message += written;
        len -= written;
    }
}

void write_reentrant_str(int filedescr, const char *message) {
    write_reentrant(filedescr, message, strlen(message));
}

void write_reentrant_char(int filedescr, const char c) {
    write_reentrant(filedescr, &c, 1);
}

void write_reentrant_int(int filedescr, int value) {
    char buffer[20];
    int len = snprintf(buffer, sizeof(buffer), "%d", value);
    if (len < 0)
        abort();
    write_reentrant(filedescr, buffer, len);
}

char read_char_reentrant(int filedescr, char *c) {
    return TEMP_FAILURE_RETRY(read(filedescr, c, 1));
}

void print_peak_memory_reentrant() {
#if OPERATING_SYSTEM == OSX || OPERATING_SYSTEM == WINDOWS || OPERATING_SYSTEM == CYGWIN
    print_peak_memory();
#else
    int proc_file_descr = TEMP_FAILURE_RETRY(open("/proc/self/status", O_RDONLY));
    if (proc_file_descr == -1) {
        write_reentrant_str(STDERR_FILENO,
                        "critical error: could not open /proc/self/status\n");
        abort();
    }

    const char magic[] = "\nVmPeak:";
    char c;
    size_t pos_magic = 0;
    const size_t len_magic = sizeof(magic) - 1;

    // Find magic word.
    while (pos_magic != len_magic && read_char_reentrant(proc_file_descr, &c) == 1) {
        if (c == magic[pos_magic]) {
            ++pos_magic;
        } else {
            pos_magic = 0;
        }
    }

    if (pos_magic != len_magic) {
        write_reentrant_str(STDERR_FILENO,
                        "critical error: could not find VmPeak in /proc/self/status\n");
        abort();
    }

    write_reentrant_str(STDOUT_FILENO, "Peak memory: ");

    // Skip over whitespace.
    while (read_char_reentrant(proc_file_descr, &c) == 1 && isspace(c))
        ;

    do {
        write_reentrant_char(STDOUT_FILENO, c);
    } while (read_char_reentrant(proc_file_descr, &c) == 1 && !isspace(c));

    write_reentrant_str(STDOUT_FILENO, " KB\n");
    TEMP_FAILURE_RETRY(close(proc_file_descr));
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

void out_of_memory_handler() {
    /*
      We do not use any memory padding currently. The methods below should
      only use stack memory. If we ever run into situations where the stack
      memory is not sufficient, we can consider using sigaltstack to reserve
      memoy for the stack of the signal handler and raising a signal here.
    */
    write_reentrant_str(STDOUT_FILENO, "Failed to allocate memory.\n");
    exit_with(EXIT_OUT_OF_MEMORY);
}

void signal_handler(int signal_number) {
    print_peak_memory_reentrant();
    write_reentrant_str(STDOUT_FILENO, "caught signal ");
    write_reentrant_int(STDOUT_FILENO, signal_number);
    write_reentrant_str(STDOUT_FILENO, " -- exiting\n");
    raise(signal_number);
}

void report_exit_code_reentrant(int exitcode) {
    const char *message;
    int filedescr = STDOUT_FILENO;
    switch (exitcode) {
    case EXIT_PLAN_FOUND:
        message = "Solution found.";
        break;
    case EXIT_CRITICAL_ERROR:
        message = "Unexplained error occurred.";
        filedescr = STDERR_FILENO;
        break;
    case EXIT_INPUT_ERROR:
        message = "Usage error occurred.";
        filedescr = STDERR_FILENO;
        break;
    case EXIT_UNSUPPORTED:
        message = "Tried to use unsupported feature.";
        filedescr = STDERR_FILENO;
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
        write_reentrant_str(STDERR_FILENO, "Exitcode: ");
        write_reentrant_int(STDERR_FILENO, exitcode);
        write_reentrant_str(STDERR_FILENO, "\nUnknown exitcode.\n");
        abort();
    }
    write_reentrant_str(filedescr, message);
    write_reentrant_char(filedescr, '\n');
}
