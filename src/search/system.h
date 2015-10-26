#ifndef SYSTEM_H
#define SYSTEM_H

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

int get_peak_memory_in_kb();
const char *get_exit_code_message_reentrant(int exitcode);
bool is_exit_code_error_reentrant(int exitcode);
void register_event_handlers();
void report_exit_code_reentrant(int exitcode);
int get_process_id();

#endif
