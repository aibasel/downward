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
# or cmake parameter DOWNWARD_CPLEX_ROOT. If different installations
# for release/debug versions of CPLEX are available,they can be
# specified with
#   DOWNWARD_CPLEX_ROOT
#   DOWNWARD_CPLEX_ROOT_RELEASE
#   DOWNWARD_CPLEX_ROOT_DEBUG
# More specific paths are preferred over less specific ones when searching
# for libraries.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

foreach(BUILDMODE "RELEASE" "DEBUG")
    set(CPLEX_HINT_PATHS_${BUILDMODE}
        ${DOWNWARD_CPLEX_ROOT_${BUILDMODE}}
        $ENV{DOWNWARD_CPLEX_ROOT_${BUILDMODE}}
        ${DOWNWARD_CPLEX_ROOT}
        $ENV{DOWNWARD_CPLEX_ROOT}
    )
endforeach()

find_path(CPLEX_INCLUDE_DIRS
    NAMES
    cplex.h
    HINTS
    ${CPLEX_HINT_PATHS_RELEASE}
    ${CPLEX_HINT_PATHS_DEBUG}
    PATH_SUFFIXES
    include/ilcplex
)

if(APPLE)
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32
        "lib/x86_osx/static_pic")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32 ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32})
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64
        "lib/x86-64_osx/static_pic")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_64 ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64})
elseif(UNIX)
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32
        "lib/x86_sles10_4.1/static_pic"
        "lib/x86_linux/static_pic")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32 ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32})
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64
        "bin/x86-64_linux/"
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
    elseif(MSVC13)
        set(CPLEX_COMPILER_HINT "vs2015")
    elseif(MSVC14)
        set(CPLEX_COMPILER_HINT "vs2017")
    endif()

    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32 "lib/x86_windows_${CPLEX_COMPILER_HINT}/stat_mda")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32 "lib/x86_windows_${CPLEX_COMPILER_HINT}/stat_mdd")
    set(CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64
      "lib/x86-64_windows_${CPLEX_COMPILER_HINT}/stat_mda"
      "lib/x64_windows_${CPLEX_COMPILER_HINT}/stat_mda")
    set(CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_64
      "lib/x86-64_windows_${CPLEX_COMPILER_HINT}/stat_mdd"
      "lib/x64_windows_${CPLEX_COMPILER_HINT}/stat_mdd")

    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(CPLEX_RUNTIME_LIBRARY_HINT "bin/x86_win32")
    elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(CPLEX_RUNTIME_LIBRARY_HINT "bin/x64_win64")
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

# CMake uses the first discovered library, searching in the order they
# are mentioned here. We prefer dynamic libraries over static ones
# (see issue925) and otherwise prefer the latest available version.
find_library(CPLEX_LIBRARY_RELEASE
    NAMES
    cplex1290
    cplex1280
    cplex1271
    cplex1262
    cplex
    HINTS
    ${CPLEX_HINT_PATHS_RELEASE}
    PATH_SUFFIXES
    ${CPLEX_LIBRARY_PATH_SUFFIX_RELEASE}
)

# See above.
find_library(CPLEX_LIBRARY_DEBUG
    NAMES
    cplex1290
    cplex1280
    cplex1271
    cplex1262
    cplex
    HINTS
    ${CPLEX_HINT_PATHS_DEBUG}
    PATH_SUFFIXES
    ${CPLEX_LIBRARY_PATH_SUFFIX_DEBUG}
)

# Parse CPLEX version.
file(STRINGS ${CPLEX_INCLUDE_DIRS}/cpxconst.h CPLEX_VERSION_STR
     REGEX "#define[ ]+CPX_VERSION[ ]+[0-9]+")
string(REGEX MATCH "[0-9]+" CPLEX_VERSION_STR ${CPLEX_VERSION_STR})
if(CPLEX_VERSION_STR)
  math(EXPR CPLEX_VERSION_MAJOR "${CPLEX_VERSION_STR} / 1000000")
  math(EXPR CPLEX_VERSION_MINOR "${CPLEX_VERSION_STR} / 10000 % 100")
  math(EXPR CPLEX_VERSION_SUBMINOR "${CPLEX_VERSION_STR} / 100 % 100")
  set(CPLEX_VERSION
      "${CPLEX_VERSION_MAJOR}.${CPLEX_VERSION_MINOR}.${CPLEX_VERSION_SUBMINOR}")
endif()

if(CPLEX_LIBRARY_RELEASE OR CPLEX_LIBRARY_DEBUG)
    find_package(Threads REQUIRED)

    set(CPLEX_LIBRARIES_COMMON ${CMAKE_THREAD_LIBS_INIT})
    if(NOT (${CPLEX_VERSION} VERSION_LESS "12.8"))
        set(CPLEX_LIBRARIES_COMMON ${CPLEX_LIBRARIES_COMMON} ${CMAKE_DL_LIBS})
    endif()

    set(CPLEX_LIBRARIES
        optimized ${CPLEX_LIBRARY_RELEASE} ${CPLEX_LIBRARIES_COMMON}
        debug ${CPLEX_LIBRARY_DEBUG} ${CPLEX_LIBRARIES_COMMON}
    )
endif()

# HACK: there must be a better way to find the dll file.
find_path(CPLEX_RUNTIME_LIBRARY_PATH
    NAMES
    cplex1262.dll
    HINTS
    ${CPLEX_HINT_PATHS_RELEASE}
    ${CPLEX_HINT_PATHS_DEBUG}
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
    CPLEX_INCLUDE_DIRS CPLEX_LIBRARIES CPLEX_LIBRARIES_COMMON CPLEX_LIBRARY_PATH_SUFFIX
    CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_32 CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_32
    CPLEX_LIBRARY_PATH_SUFFIX_RELEASE_64 CPLEX_LIBRARY_PATH_SUFFIX_DEBUG_64
    CPLEX_LIBRARY_PATH_SUFFIX_RELEASE CPLEX_LIBRARY_PATH_SUFFIX_DEBUG
    CPLEX_LIBRARY_RELEASE CPLEX_LIBRARY_DEBUG CPLEX_RUNTIME_LIBRARY_PATH
    CPX_VERSION CPLEX_VERSION_MAJOR CPLEX_VERSION_MINOR CPLEX_VERSION_STR
    CPLEX_VERSION_SUBMINOR
)
