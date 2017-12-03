#ifndef UTILS_SYSTEM_H
#define UTILS_SYSTEM_H

#define LINUX 0
#define OSX 1
#define WINDOWS 2

#if defined(_WIN32)
#define OPERATING_SYSTEM WINDOWS
#include "system_windows.h"
#elif defined(__APPLE__)
#define OPERATING_SYSTEM OSX
#include "system_unix.h"
#else
#define OPERATING_SYSTEM LINUX
#include "system_unix.h"
#endif

#include "language.h"

#include <iostream>

#define ABORT(msg) \
    ( \
        (std::cerr << "Critical error in file " << __FILE__ \
                   << ", line " << __LINE__ << ": " << std::endl \
                   << (msg) << std::endl), \
        (abort()), \
        (void)0 \
    )

namespace utils {
enum class ExitCode {
    /*
      We document Fast Downward exit codes at
      http://www.fast-downward.org/ExitCodes. Please update this documentation
      when making changes below.
    */
    // 0-9: exit codes denoting a plan was found
    SUCCESS = 0,

    // 10-19: exit codes denoting no plan was found (without any error)
    SEARCH_UNSOLVABLE = 11,  // Task is provably unsolvable with given bound.
    SEARCH_UNSOLVED_INCOMPLETE = 12,  // Search ended without finding a solution.

    // 20-29: "expected" failures
    SEARCH_OUT_OF_MEMORY = 22,
    SEARCH_OUT_OF_TIME = 23,

    // 30-39: unrecoverable errors
    SEARCH_CRITICAL_ERROR = 32,
    SEARCH_INPUT_ERROR = 33,
    SEARCH_UNSUPPORTED = 34
};

NO_RETURN extern void exit_with(ExitCode returncode);
NO_RETURN extern void exit_after_receiving_signal(ExitCode returncode);

int get_peak_memory_in_kb();
const char *get_exit_code_message_reentrant(ExitCode exitcode);
bool is_exit_code_error_reentrant(ExitCode exitcode);
void register_event_handlers();
void report_exit_code_reentrant(ExitCode exitcode);
int get_process_id();
}

#endif
