# - Find the Open Solver Interface by COIN/OR.
# This module will search for the OSI proxies for the solvers
# specified as components in the FIND_PACKAGE call.
# Possible solvers are:
#
#  Clp (internal solver of COIN)
#  Cpx (CPLEX)
#  Grb (Gurobi)
#
# This code defines the following variables:
#
#  OSI_FOUND                 - TRUE if all components are found.
#  OSI_INCLUDE_DIRS          - Full paths to all include dirs.
#  OSI_LIBRARY               - Full paths to the OSI library.
#  OSI_LIBRARIES             - Full paths to all libraries.
#  OSI_<solver>_FOUND        - TRUE if a proxy for <solver> is found.
#  OSI_<solver>_LIBRARIES    - Full path to proxy and main libraries for <solver>.
#
# Example Usages:
#  find_package(OSI)
#  find_package(OSI COMPONENTS Clp Cpx)
#
# Note that the standard FIND_PACKAGE features are supported
# (i.e., QUIET, REQUIRED, etc.).

set(_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
if(DEFINED ENV{DOWNWARD_COIN_ROOT})
    set(CMAKE_FIND_ROOT_PATH $ENV{DOWNWARD_COIN_ROOT})
    set(_COIN_ROOT_OPTS ONLY_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH)
endif()

find_path(OSI_INCLUDES
    NAMES
    OsiConfig.h config_osi.h
    PATHS
    /include/coin
    ${_COIN_ROOT_OPTS}
)

set(OSI_LIBRARIES "")
foreach(solver ${OSI_FIND_COMPONENTS})
    set(OSI_${solver}_FOUND FALSE)
    find_library(OSI_${solver}_LIBRARY
        Osi${solver}
        PATHS
        /lib
        ${_COIN_ROOT_OPTS}
    )
    set(OSI_${solver}_LIBRARIES ${OSI_${solver}_LIBRARY})
    if(OSI_${solver}_LIBRARIES)
        list(APPEND OSI_LIBRARIES ${OSI_${solver}_LIBRARIES})
        set(OSI_${solver}_FOUND TRUE)
    endif()
    mark_as_advanced(OSI_${solver}_LIBRARY)
endforeach()

find_library(OSI_LIBRARY
    Osi
    PATHS
    /lib
    ${_COIN_ROOT_OPTS}
)

## Basic Osi libs must be added after (!) all osi solver libs
list(APPEND OSI_LIBRARIES ${OSI_LIBRARY})

# Restore the original search paths
set(CMAKE_FIND_ROOT_PATH ${_CMAKE_FIND_ROOT_PATH})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    OSI
    REQUIRED_VARS OSI_INCLUDES OSI_LIBRARIES
    HANDLE_COMPONENTS
)

mark_as_advanced(OSI_INCLUDES OSI_LIBRARY OSI_LIBRARIES)
