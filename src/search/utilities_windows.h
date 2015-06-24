#ifndef UTILITIES_WINDOWS_H
#define UTILITIES_WINDOWS_H

// avoid min/max conflicts http://support.microsoft.com/kb/143208
#ifndef NOMINMAX
#define NOMINMAX
#endif

// speed up build process by skipping some includes https://support.microsoft.com/de-ch/kb/166474
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#endif
