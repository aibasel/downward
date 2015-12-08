# - Find the CLP LP solver.
# This code defines the following variables:
#
#  CLP_FOUND                 - TRUE if CLP was found.
#  CLP_INCLUDE_DIRS          - Full paths to all include dirs.
#  CLP_LIBRARIES             - Full paths to all libraries.
#
# Usage:
#  find_package(clp)
#
# Hints to the location of CLP can be specified using the variables
# COIN_HINT_PATHS_RELEASE and COIN_HINT_PATHS_DEBUG. This is used by
# FindOsi to locate the CLP version that is shipped with the OSI version
# which was found.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

foreach(BUILDMODE "RELEASE" "DEBUG")
    set(_CMAKE_FIND_ROOT_PATH_${BUILDMODE} ${CMAKE_FIND_ROOT_PATH})
    if(COIN_HINT_PATHS_${BUILDMODE})
        set(_CMAKE_FIND_ROOT_PATH_${BUILDMODE} ${COIN_HINT_PATHS_${BUILDMODE}})
        set(_COIN_ROOT_OPTS_${BUILDMODE} ONLY_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH)
    endif()

    find_path(CLP_INCLUDE_DIRS
        NAMES ClpSimplex.hpp
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES include/coin
        ${_COIN_ROOT_OPTS_${BUILDMODE}}
    )
    find_library(CLP_LIBRARY_${BUILDMODE}
        NAMES Clp libClp
        HINTS ${_CMAKE_FIND_ROOT_PATH_${BUILDMODE}}
        PATH_SUFFIXES lib
        ${_COIN_ROOT_OPTS_${BUILDMODE}}
    )
endforeach()

set(CLP_LIBRARIES
    optimized ${CLP_LIBRARY_RELEASE}
    debug ${CLP_LIBRARY_DEBUG})

# Check if everything was found and set CLP_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Clp
    REQUIRED_VARS CLP_INCLUDE_DIRS CLP_LIBRARIES
)

mark_as_advanced(CLP_LIBRARY_RELEASE CLP_LIBRARY_DEBUG CLP_INCLUDE_DIRS CLP_LIBRARIES)
