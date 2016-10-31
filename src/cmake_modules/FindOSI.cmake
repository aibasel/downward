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
#  OSI_LIBRARIES             - Full paths to all libraries.
#  OSI_<solver>_FOUND        - TRUE if a proxy for <solver> is found.
#  OSI_<solver>_INCLUDE_DIRS - Full path to include directories for <solver>.
#  OSI_<solver>_LIBRARIES    - Full path to proxy and main libraries for <solver>.
#
# Example Usages:
#  find_package(OSI)
#  find_package(OSI COMPONENTS Clp Cpx)
#
# The location of OSI can be specified using the environment variable
# or cmake parameter DOWNWARD_COIN_ROOT. If different installations
# for 32-/64-bit versions and release/debug versions of OSI are available,
# they can be specified with
#   DOWNWARD_COIN_ROOT32
#   DOWNWARD_COIN_ROOT64
#   DOWNWARD_COIN_ROOT_RELEASE32
#   DOWNWARD_COIN_ROOT_RELEASE64
#   DOWNWARD_COIN_ROOT_DEBUG32
#   DOWNWARD_COIN_ROOT_DEBUG64
# More specific paths are preferred over less specific ones when searching
# for libraries.
#
# Note that the standard FIND_PACKAGE features are supported
# (QUIET, REQUIRED, etc.).

foreach(BITWIDTH 32 64)
    foreach(BUILDMODE "RELEASE" "DEBUG")
        set(COIN_HINT_PATHS_${BUILDMODE}${BITWIDTH}
            ${DOWNWARD_COIN_ROOT_${BUILDMODE}${BITWIDTH}}
            $ENV{DOWNWARD_COIN_ROOT_${BUILDMODE}${BITWIDTH}}
            ${DOWNWARD_COIN_ROOT${BITWIDTH}}
            $ENV{DOWNWARD_COIN_ROOT${BITWIDTH}}
            ${DOWNWARD_COIN_ROOT}
            $ENV{DOWNWARD_COIN_ROOT}
        )
    endforeach()
endforeach()

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(COIN_HINT_PATHS_RELEASE ${COIN_HINT_PATHS_RELEASE32})
    set(COIN_HINT_PATHS_DEBUG ${COIN_HINT_PATHS_DEBUG32})
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(COIN_HINT_PATHS_RELEASE ${COIN_HINT_PATHS_RELEASE64})
    set(COIN_HINT_PATHS_DEBUG ${COIN_HINT_PATHS_DEBUG64})
else()
    message(WARNING "Bitwidth could not be detected, preferring 32-bit version of COIN")
    set(COIN_HINT_PATHS_RELEASE
        ${COIN_HINT_PATHS_RELEASE32}
        ${COIN_HINT_PATHS_RELEASE64}
    )
    set(COIN_HINT_PATHS_DEBUG
        ${COIN_HINT_PATHS_DEBUG32}
        ${COIN_HINT_PATHS_DEBUG64}
    )
endif()


# Find include dirs.
find_path(OSI_INCLUDE_DIRS
    NAMES OsiConfig.h config_osi.h CoinPragma.hpp
    HINTS ${COIN_HINT_PATHS_RELEASE} ${COIN_HINT_PATHS_DEBUG}
    PATH_SUFFIXES include include/coin
)

# Find main libraries.
find_library(OSI_COINUTILS_LIBRARY_RELEASE
    NAMES CoinUtils libCoinUtils
    HINTS ${COIN_HINT_PATHS_RELEASE}
    PATH_SUFFIXES lib
)
find_library(OSI_COINUTILS_LIBRARY_DEBUG
    NAMES CoinUtils libCoinUtils
    HINTS ${COIN_HINT_PATHS_DEBUG}
    PATH_SUFFIXES lib
)

find_library(OSI_LIBRARY_RELEASE
    NAMES Osi libOsi
    HINTS ${COIN_HINT_PATHS_RELEASE}
    PATH_SUFFIXES lib
)
find_library(OSI_LIBRARY_DEBUG
    NAMES Osi libOsi
    HINTS ${COIN_HINT_PATHS_DEBUG}
    PATH_SUFFIXES lib
)

set(OSI_LIBRARIES
    optimized ${OSI_LIBRARY_RELEASE}
    optimized ${OSI_COINUTILS_LIBRARY_RELEASE}
    debug ${OSI_LIBRARY_DEBUG}
    debug ${OSI_COINUTILS_LIBRARY_DEBUG})

# Find solver proxies.
foreach(SOLVER ${OSI_FIND_COMPONENTS})
    set(OSI_${SOLVER}_FOUND FALSE)
    find_library(OSI_${SOLVER}_LIBRARY_RELEASE
        NAMES Osi${SOLVER} libOsi${SOLVER}
        HINTS ${COIN_HINT_PATHS_RELEASE}
        PATH_SUFFIXES lib
    )
    find_library(OSI_${SOLVER}_LIBRARY_DEBUG
        NAMES Osi${SOLVER} libOsi${SOLVER}
        HINTS ${COIN_HINT_PATHS_DEBUG}
        PATH_SUFFIXES lib
    )
    set(OSI_${SOLVER}_LIBRARIES
        optimized ${OSI_${SOLVER}_LIBRARY_RELEASE}
        debug ${OSI_${SOLVER}_LIBRARY_DEBUG})
endforeach()

# A component is present if its adapter is present and its solver is present.

# Clp component
if(OSI_Clp_LIBRARIES)
    find_package(Clp)
    if (CLP_FOUND)
        list(APPEND OSI_Clp_LIBRARIES ${CLP_LIBRARIES})
        list(APPEND OSI_Clp_INCLUDE_DIRS ${CLP_INCLUDE_DIRS})
        set(OSI_Clp_FOUND TRUE)
    endif()
endif()

# Cpx component
if(OSI_Cpx_LIBRARIES)
    find_package(Cplex)
    if (CPLEX_FOUND)
        list(APPEND OSI_Cpx_LIBRARIES ${CPLEX_LIBRARIES})
        list(APPEND OSI_Cpx_INCLUDE_DIRS ${CPLEX_INCLUDE_DIRS})
        set(OSI_Cpx_FOUND TRUE)
    endif()
endif()

# Grb component
if(OSI_Grb_LIBRARIES)
    message(FATAL_ERROR "Gurobi is not tested yet. Try to copy the config of cplex above and the file cmake/modules/FindCplex.cmake and adapt them for gurobi.")
    set(OSI_Grb_FOUND FALSE)
endif()


# Check for consistency and handle arguments like QUIET, REQUIRED, etc.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    OSI
    REQUIRED_VARS OSI_INCLUDE_DIRS OSI_LIBRARIES
    HANDLE_COMPONENTS
)

# Do not show internal variables in cmake GUIs like ccmake.
mark_as_advanced(OSI_INCLUDE_DIRS OSI_LIBRARY_RELEASE OSI_LIBRARY_DEBUG
                 OSI_COINUTILS_LIBRARY_RELEASE OSI_COINUTILS_LIBRARY_DEBUG
                 OSI_LIBRARIES COIN_HINT_PATHS_RELEASE COIN_HINT_PATHS_DEBUG)
foreach(SOLVER ${OSI_FIND_COMPONENTS})
    mark_as_advanced(OSI_${SOLVER}_LIBRARY_RELEASE OSI_${SOLVER}_LIBRARY_DEBUG
                     OSI_${SOLVER}_LIBRARIES OSI_${SOLVER}_INCLUDE_DIRS)
endforeach()
