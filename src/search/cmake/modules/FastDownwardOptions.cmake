## Variables prefixed with "DOWNWARD_" are used to configure the
## build. They can be changed as cmake arguments,
## e.g. -DDOWNWARD_BITWIDTH=64 or set with the ccmake GUI.

option(DOWNWARD_USE_LP
       "Enable linear programming stuff."
       NO)

option(DOWNWARD_LINK_RELEASE_STATICALLY
       "Set DOWNWARD_LINK_RELEASE_STATICALLY to 0 or 1 (default) to disable/enable static linking of the executable in release mode. On OS X, this is unsupported and will be silently disabled."
       YES)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(DOWNWARD_LINK_RELEASE_STATICALLY NO)
endif()
