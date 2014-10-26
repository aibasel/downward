#if OPERATING_SYSTEM == LINUX
#include "reentrant.h"

/*
  NOTE:
  Methods in this file are meant to be used in event handlers. They
  should all be "re-entrant", i.e. they must not use static variables,
  global data, or locks. Only some system calls such as
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

#include "utilities.h"

#include <csignal>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;


void write_reentrant(int filedescr, const char *message, int len) {
    while (len > 0) {
        int written = TEMP_FAILURE_RETRY(write(filedescr, message, len));
        /*
          We could check the value of errno here but all errors except EINTR
          are catastrophic enough to abort, so we do not need the distintion.
          The error EINTR is handled by the macro TEMP_FAILURE_RETRY.
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
    int result = TEMP_FAILURE_RETRY(read(filedescr, c, 1));
    /*
      We could check the value of errno here but all errors except EINTR
      are catastrophic enough to abort, so we do not need the distintion.
      The error EINTR is handled by the macro TEMP_FAILURE_RETRY.
    */
    if (result == -1)
        abort();
    return result == 1;
}

void print_peak_memory_reentrant() {
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
}

void exit_handler(int, void *) {
    print_peak_memory_reentrant();
}

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
        write_reentrant_str(STDERR_FILENO, "Exitcode: ");
        write_reentrant_int(STDERR_FILENO, exitcode);
        write_reentrant_str(STDERR_FILENO, "\nUnknown exitcode.\n");
        abort();
    }
    int filedescr = is_error ? STDERR_FILENO : STDOUT_FILENO;
    write_reentrant_str(filedescr, message);
    write_reentrant_char(filedescr, '\n');
}
#endif
