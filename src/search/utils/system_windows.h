#ifndef UTILS_SYSTEM_WINDOWS_H
#define UTILS_SYSTEM_WINDOWS_H

#include "system.h"

#if OPERATING_SYSTEM == WINDOWS

// Avoid min/max conflicts (http://support.microsoft.com/kb/143208).
#ifndef NOMINMAX
#define NOMINMAX
#endif

/* Speed up build process by skipping some includes
   (https://support.microsoft.com/de-ch/kb/166474). */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#endif
#endif
