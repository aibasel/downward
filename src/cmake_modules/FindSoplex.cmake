# - Find the SoPlex LP solver.
# This code defines the following variables:
#
#  SOPLEX_FOUND                 - TRUE if SOPLEX was found.
#  SOPLEX_INCLUDE_DIRS          - Full paths to all include dirs.
#  SOPLEX_LIBRARIES             - Full paths to all libraries.
#
# Usage:
#  find_package(soplex)
#
# The location of SoPlex can be specified using the environment variable
# or cmake parameter DOWNWARD_SOPLEX_ROOT. If different installations
# for release/debug versions of SOPLEX are available, they can be
# specified with
#   DOWNWARD_SOPLEX_ROOT
#   DOWNWARD_SOPLEX_ROOT_RELEASE
#   DOWNWARD_SOPLEX_ROOT_DEBUG
# More specific paths are preferred over less specific ones when searching
# for libraries.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

foreach(BUILDMODE "RELEASE" "DEBUG")
    set(SOPLEX_HINT_PATHS_${BUILDMODE}
        ${DOWNWARD_SOPLEX_ROOT_${BUILDMODE}}
        $ENV{DOWNWARD_SOPLEX_ROOT_${BUILDMODE}}
        ${DOWNWARD_SOPLEX_ROOT}
        $ENV{DOWNWARD_SOPLEX_ROOT}
    )
endforeach()

find_path(SOPLEX_INCLUDE_DIRS
    NAMES
    soplex.h
    HINTS
    ${SOPLEX_HINT_PATHS_RELEASE}
    ${SOPLEX_HINT_PATHS_DEBUG}
    PATH_SUFFIXES
    include
)

find_library(SOPLEX_LIBRARY_RELEASE
    NAMES
    soplex
    HINTS
    ${SOPLEX_HINT_PATHS_RELEASE}
    PATH_SUFFIXES
    lib
)

find_library(SOPLEX_LIBRARY_DEBUG
    NAMES
    soplex
    HINTS
    ${SOPLEX_HINT_PATHS_DEBUG}
    PATH_SUFFIXES
    lib
)

if(SOPLEX_LIBRARY_RELEASE OR SOPLEX_LIBRARY_DEBUG)
    set(SOPLEX_LIBRARIES
        optimized ${SOPLEX_LIBRARY_RELEASE}
        debug ${SOPLEX_LIBRARY_DEBUG}
    )
endif()

# Check if everything was found and set SOPLEX_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    soplex
    REQUIRED_VARS SOPLEX_INCLUDE_DIRS SOPLEX_LIBRARIES
)

mark_as_advanced(
    SOPLEX_INCLUDE_DIRS SOPLEX_LIBRARIES
    SOPLEX_LIBRARY_RELEASE SOPLEX_LIBRARY_DEBUG
)
