#ifndef SYSTEM_H
#define SYSTEM_H

#define LINUX 0
#define OSX 1
#define CYGWIN 2
#define WINDOWS 3

#if defined(__CYGWIN32__)
#define OPERATING_SYSTEM CYGWIN
#elif defined(__WINNT__)
#define OPERATING_SYSTEM WINDOWS
#elif defined(__APPLE__)
#define OPERATING_SYSTEM OSX
#else
#define OPERATING_SYSTEM LINUX
#endif

double current_system_clock();
double current_system_clock_exact();
int get_peak_memory_in_kb();
extern void register_event_handlers();
void report_exit_code_reentrant(int exitcode);

#endif
