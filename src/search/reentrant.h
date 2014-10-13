#ifndef REENTRANT_H
#define REENTRANT_H

#if OPERATING_SYSTEM == LINUX
void exit_handler(int exit_code, void *hint);
#elif OPERATING_SYSTEM == OSX
void exit_handler();
#include <mach/mach.h>
#elif OPERATING_SYSTEM == WINDOWS || OPERATING_SYSTEM == CYGWIN
#include <windows.h>
#include <psapi.h>
#endif

void out_of_memory_handler();
extern "C" void signal_handler(int signal_number);
void report_exit_code_reentrant(int exitcode);

#endif
