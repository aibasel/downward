## Variables prefixed with "DOWNWARD_" are used to configure the
## build. They can be changed as cmake arguments,
## e.g. -DDOWNWARD_BITWIDTH=64 or set with the ccmake GUI.

set(DOWNWARD_BITWIDTH
    "32"
    CACHE STRING
    "By default, build in 32-bit mode. Set to '64' to build in 64-bit mode and to 'native' to use the native bitness of the OS.")
if(${DOWNWARD_BITWIDTH} STREQUAL "32")
    set(BITWIDTHOPT "-m32")
elseif (${DOWNWARD_BITWIDTH} STREQUAL "64")
    set(BITWIDTHOPT "-m64")
elseif (NOT ${DOWNWARD_BITWIDTH} STREQUAL "native")
    message( FATAL_ERROR "Bad value for DOWNWARD_BITWIDTH." )
endif(${DOWNWARD_BITWIDTH} STREQUAL "32")

option(DOWNWARD_USE_LP
       "Enable linear programming stuff."
       NO)

option(DOWNWARD_LINK_RELEASE_STATICALLY
       "Set DOWNWARD_LINK_RELEASE_STATICALLY to 0 or 1 (default) to disable/enable static linking of the executable in release mode. On OS X, this is unsupported and will be silently disabled."
       YES)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(DOWNWARD_LINK_RELEASE_STATICALLY NO)
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
