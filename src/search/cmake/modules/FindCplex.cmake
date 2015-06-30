# - Find the CPlex LP solver.
# This code defines the following variables:
#
#  CPLEX_FOUND                 - TRUE if Cplex was found.
#  CPLEX_INCLUDE_DIRS          - Full paths to all include dirs.
#  CPLEX_LIBRARIES             - Full paths to all libraries.
#
# Usage:
#  find_package(cplex)
#
# Note that the standard FIND_PACKAGE features are supported
# (i.e., QUIET, REQUIRED, etc.).

find_path(CPLEX_INCLUDE_DIRS
    NAMES
    cplex.h
    HINTS
    $ENV{DOWNWARD_CPLEX_ROOT}
    ${DOWNWARD_CPLEX_ROOT}
    PATH_SUFFIXES
    include/ilcplex
)

#TODO: version and path detection
if(UNIX)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(CPLEX_LIBRARY_PATH_SUFFIX
            "lib/x86_sles10_4.1/static_pic"
            "lib/x86_linux/static_pic")
    elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(CPLEX_LIBRARY_PATH_SUFFIX
            "lib/x86-64_sles10_4.1/static_pic"
            "lib/x86-64_linux/static_pic")
    else()
        message(WARNING "Bitwidth could not be detected, preferring 32bit version of Cplex")
        set(CPLEX_LIBRARY_PATH_SUFFIX
            "lib/x86_sles10_4.1/static_pic"
            "lib/x86_linux/static_pic"
            "lib/x86-64_sles10_4.1/static_pic"
            "lib/x86-64_linux/static_pic")
    endif()
elseif(MSVC)
    set(CPLEX_LIBRARY_PATH_SUFFIX "lib/x86_windows_vs2013/stat_mda")
endif()

find_library(CPLEX_LIBRARIES
    NAMES
    cplex
    cplex1262
    HINTS
    $ENV{DOWNWARD_CPLEX_ROOT}
    ${DOWNWARD_CPLEX_ROOT}
    PATH_SUFFIXES
    ${CPLEX_LIBRARY_PATH_SUFFIX}
)

if(CPLEX_LIBRARIES)
    find_package(Threads REQUIRED)
    set(CPLEX_LIBRARIES ${CPLEX_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Cplex
    REQUIRED_VARS CPLEX_INCLUDE_DIRS CPLEX_LIBRARIES
)

mark_as_advanced(CPLEX_INCLUDE_DIRS CPLEX_LIBRARIES CPLEX_LIBRARY_PATH_SUFFIX)
