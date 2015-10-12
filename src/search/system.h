#ifndef SYSTEM_H
#define SYSTEM_H

#define LINUX 0
#define OSX 1
#define CYGWIN 2
#define WINDOWS 3

#if defined(__CYGWIN32__)
#define OPERATING_SYSTEM CYGWIN
#include "system_windows.h"
#elif defined(_WIN32)
#define OPERATING_SYSTEM WINDOWS
#include "system_windows.h"
#elif defined(__APPLE__)
#define OPERATING_SYSTEM OSX
#include "system_unix.h"
#else
#define OPERATING_SYSTEM LINUX
#include "system_unix.h"
#endif

int get_peak_memory_in_kb();
extern void register_event_handlers();
void report_exit_code_reentrant(int exitcode);
int get_process_id();

#endif
