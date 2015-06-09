
/* avoid min/max conflicts (see http://support.microsoft.com/kb/143208) */
#ifndef NOMINMAX
#define NOMINMAX
#endif

#if _MSC_VER >= 1700
#include <../um/windows.h>
#else
#include <../include/windows.h>
#endif
