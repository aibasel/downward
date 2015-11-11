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
# The location of CLP can be specified using the environment variable
# or cmake parameter DOWNWARD_COIN_ROOT.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

set(_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
if(DEFINED ENV{DOWNWARD_COIN_ROOT})
    set(_CMAKE_FIND_ROOT_PATH $ENV{DOWNWARD_COIN_ROOT})
    set(_COIN_ROOT_OPTS ONLY_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH)
endif()

find_path(CLP_INCLUDE_DIRS
    NAMES ClpSimplex.hpp
    HINTS ${_CMAKE_FIND_ROOT_PATH}/include/coin
    ${_COIN_ROOT_OPTS}
)

find_library(CLP_LIBRARIES
    NAMES Clp
    HINTS ${_CMAKE_FIND_ROOT_PATH}/lib
    ${_COIN_ROOT_OPTS}
)

# Check if everything was found and set CLP_FOUND.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Clp
    REQUIRED_VARS CLP_INCLUDE_DIRS CLP_LIBRARIES
)

mark_as_advanced(CLP_INCLUDE_DIRS CLP_LIBRARIES)
