# - Find the CPLEX LP solver.
# This code defines the following variables:
#
#  CPLEX_FOUND                 - TRUE if CPLEX was found.
#  CPLEX_INCLUDE_DIRS          - Full paths to all include dirs.
#  CPLEX_LIBRARIES             - Full paths to all libraries.
#  CPLEX_RUNTIME_LIBRARY       - Full path to the dll file on windows
#
# Usage:
#  find_package(cplex)
#
# The location of CPLEX can be specified using the environment variable
# or cmake parameter DOWNWARD_CPLEX_ROOT.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

find_path(CPLEX_INCLUDE_DIRS
    NAMES
    cplex.h
    HINTS
    $ENV{DOWNWARD_CPLEX_ROOT}
    ${DOWNWARD_CPLEX_ROOT}
    PATH_SUFFIXES
    include/ilcplex
)

if(UNIX)
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32
        "lib/x86_sles10_4.1/static_pic"
        "lib/x86_linux/static_pic")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32 ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32})
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64
        "lib/x86-64_sles10_4.1/static_pic"
        "lib/x86-64_linux/static_pic")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_64 ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64})
elseif(MSVC)
    # Note that the numbers are correct: Visual Studio 2011 is version 10.
    if (MSVC10)
        set(CPLEX_COMPILER_HINT "vs2011")
    elseif(MSVC11)
        set(CPLEX_COMPILER_HINT "vs2012")
    elseif(MSVC12)
        set(CPLEX_COMPILER_HINT "vs2013")
    endif()

    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32 "lib/x86_windows_${CPLEX_COMPILER_HINT}/stat_mda")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32 "lib/x86_windows_${CPLEX_COMPILER_HINT}/stat_mdd")
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64 "lib/x86-64_windows_${CPLEX_COMPILER_HINT}/stat_mda")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_64 "lib/x86-64_windows_${CPLEX_COMPILER_HINT}/stat_mdd")
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(CPLEX_RUNTIME_LIBRARY_HINT "bin/x86_win32")
    elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(CPLEX_RUNTIME_LIBRARY_HINT "bin/x86_win64")
    endif()
endif()

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32})
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG ${CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32})
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64})
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG ${CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_64})
else()
    message(WARNING "Bitwidth could not be detected, preferring 32bit version of CPLEX")
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE
        ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32}
        ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64}
    )
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG
        ${CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32}
        ${CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_64}
    )
endif()

find_library(CPLEX_LIBRARY_RELEASE
    NAMES
    cplex
    cplex1262
    HINTS
    $ENV{DOWNWARD_CPLEX_ROOT}
    ${DOWNWARD_CPLEX_ROOT}
    PATH_SUFFIXES
    ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE}
)

find_library(CPLEX_LIBRARY_DEBUG
    NAMES
    cplex
    cplex1262
    HINTS
    $ENV{DOWNWARD_CPLEX_ROOT}
    ${DOWNWARD_CPLEX_ROOT}
    PATH_SUFFIXES
    ${CPLEX_LIBRARY_PATH_SUFFIX_DEBUG}
)

if(CPLEX_LIBRARY_RELEASE OR CPLEX_LIBRARY_DEBUG)
    find_package(Threads REQUIRED)
    set(CPLEX_LIBRARIES
        optimized ${CPLEX_LIBRARY_RELEASE} ${CMAKE_THREAD_LIBS_INIT}
        debug ${CPLEX_LIBRARY_DEBUG} ${CMAKE_THREAD_LIBS_INIT}
    )
endif()

# HACK: there must be a better way to find the dll file.
find_path(CPLEX_RUNTIME_LIBRARY_PATH
    NAMES
    cplex1262.dll
    HINTS
    $ENV{DOWNWARD_CPLEX_ROOT}
    ${DOWNWARD_CPLEX_ROOT}
    PATH_SUFFIXES
    ${CPLEX_RUNTIME_LIBRARY_HINT}
)
if(CPLEX_RUNTIME_LIBRARY_PATH)
    set(CPLEX_RUNTIME_LIBRARY "${CPLEX_RUNTIME_LIBRARY_PATH}/cplex1262.dll")
endif()

# Check if everything was found and set CPLEX_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Cplex
    REQUIRED_VARS CPLEX_INCLUDE_DIRS CPLEX_LIBRARIES
)

mark_as_advanced(
    CPLEX_INCLUDE_DIRS CPLEX_LIBRARIES CPLEX_LIBRARY_PATH_SUFFIX
    CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32 CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32
    CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64 CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_64
    CPLEX_LIBRARY_PATH_SUFFIX_RELEASE CPLEX_LIBRARY_PATH_SUFFIX_DEBUG
    CPLEX_LIBRARY_RELEASE CPLEX_LIBRARY_DEBUG CPLEX_RUNTIME_LIBRARY_PATH
)
