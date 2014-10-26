# Version 2.6 introduces functions.
cmake_minimum_required(VERSION 2.6)

# Default to Release build, unless specified otherwise
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile")
if(NOT CMAKE_BUILD_TYPE)
    message("Defaulting to release build.")
    set(CMAKE_BUILD_TYPE Release
        CACHE STRING
        "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
        FORCE)
endif()

# Default tool chain forces a 32 bit compilation
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message("Defaulting to 32 bit compilation.")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/toolchain-32bit.cmake")
endif()

set(TARGET "downward")

project(${TARGET})

### Fast Downward CMake build file
### Unless someone adopts it, queries/comments to Andrew.Coles@cis.strath.ac.uk
###
###   * Usage: cmake . ; make [-j2]
###   * The first command creates and stores options in the file CMakeCache.txt.
###     Changing the default values for the options here has no effect, you need
###     to change them in CMakeCache.txt or pass -DMY_OPTION to cmake.

## == Path containing some helper CMake modules ==
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

option(DOWNWARD_LINK_RELEASE_STATICALLY
       "Set DOWNWARD_LINK_RELEASE_STATICALLY to 0 or 1 (default) to disable/enable static linking of the executable in release mode. On OS X, this is unsupported and will be silently disabled."
       YES)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(DOWNWARD_LINK_RELEASE_STATICALLY NO)
endif()

include("FastDownwardPlugins")

# First, make a lower-case version of the build type
string(TOLOWER "${CMAKE_BUILD_TYPE}" LOWER_CASE_BUILD_TYPE)

# Use distinct names for the different targets
set(TARGET "${TARGET}-${LOWER_CASE_BUILD_TYPE}")

## == Definition of build flags ==
##
## You probably won't need to change this system, unless you want to tweak it for a specific
## platform.  By default, four build targets are defined:
##
## * -DCMAKE_BUILD_TYPE=Debug          - no optimisation, full debugging information
## * -DCMAKE_BUILD_TYPE=Release        - -O3 optimisation, static binary, no debugging information
## * -DCMAKE_BUILD_TYPE=RelWithDebInfo - -O3 optimisation, static binary, but with debugging information
## * -DCMAKE_BUILD_TYPE=Profile        - -O3 optimisation, profile information linked in
##
## We overwrite the default configuration to include "-g", allow cross
## compilation and switch to pedantic error reporting.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
# Note: we write "-std=c++0x" rather than "-std=c++11" to support gcc 4.4.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-deprecated -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/ext")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    ## If you have Mac OS, you may be getting tree.hh from /opt/local/include.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/local/include/")
endif()

## The following lines contain workarounds for bugs when
## cross-compiling to 64 bit on 32-bit systems using gcc 4.4 or gcc
## 4.5 in some Ubuntu releases. (We don't usually cross-compile to
## 64-bit, but in some cases we do; e.g. we did for the IPC.) See
## http://stackoverflow.com/questions/4643197/missing-include-bits-cconfig-h-when-cross-compiling-64-bit-program-on-32-bit.
if(CMAKE_COMPILER_IS_GNUCXX)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
                    OUTPUT_VARIABLE GCC_VERSION)
    string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
    if(GCC_VERSION VERSION_LESS 4.6)
        list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
        list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)
        message(STATUS "Version ${GCC_MAJOR}.${GCC_MINOR} < 4.6")
        # If the original list wasn't part of the string the result will be two list items.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/include/c++/${GCC_MAJOR}.${GCC_MINOR}/i686-linux-gnu")
    endif()
endif()

set(CMAKE_CXX_LINK_FLAGS "")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${BITWIDTHOPT}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -g")


## Configuration-specific options
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-O3")
set(CMAKE_CXX_FLAGS_PROFILE "-O3 -pg")

set(CMAKE_CXX_LINK_FLAGS_PROFILE "-pg")

# Here, we modify the default Release build compile flags to make a static executable
# This is done slightly differently for different compilers: the -static-libgcc flag
# makes no sense if using the Intel compiler.
if(DOWNWARD_LINK_RELEASE_STATICALLY)
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_LINK_FLAGS_RELEASE "${CMAKE_CXX_LINK_FLAGS_RELEASE} -static-libgcc")
    endif()

    if(${LOWER_CASE_BUILD_TYPE} STREQUAL "release")
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
        else()
            set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
            message(WARNING "Static release build is only tested on linux.")
        endif()
    endif()
endif()

## == Defining the executable target ==

add_executable(${TARGET}
    planner.cc
    ${search_main_SRCS}
    ${search_plugin_SRCS}
)


# Finally, find the rt library for the exact timer
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    find_library(RT_LIBRARY rt)
    target_link_libraries(${TARGET} ${RT_LIBRARY})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    find_library(PSAPI_LIBRARY psapi)
    target_link_libraries(${TARGET} ${PSAPI_LIBRARY})
endif()


## == Finding the LP libraries ==
## We want to link the Linear Programming libraries statically since
## they are unlikely to be preinstalled on the grids we use for
## evaluation. Static linking is a bit tricky: we need to specify the
## libraries *after* the source files and in such an order that if A
## depends on B, A is listed before B. (In case of dependency cycles,
## we can and must list them multiple times.) The following set of
## libraries and their ordering have been determined experimentally
## and hence might break if we use more functions from the LP
## libraries. See
## http://ask.metafilter.com/117792/How-to-fix-C-static-linking-problems

## We want to always link the COIN libraries statically, even if static
## linking is otherwise disabled. We accomplish this by
## using -Wl,-Bstatic before the COIN libs and -Wl,-Bdynamic
## afterwards (unless in release mode with static linking enabled). See
## http://ubuntuforums.org/showthread.php?t=491455
set(_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

find_package(OSI OPTIONAL_COMPONENTS Cpx Clp Grb)
find_package(CoinUtils)

if(OSI_FOUND AND COINUTILS_FOUND)
    add_definitions("-D USE_LP")
    include_directories(${OSI_INCLUDES} ${COINUTILS_INCLUDES})
    target_link_libraries(${TARGET} ${OSI_LIBRARIES} ${COINUTILS_LIBRARIES})

    if(OSI_Cpx_FOUND)
        find_package(Cplex)
        if (CPLEX_FOUND)
            message("cplex")
            add_definitions("-D COIN_HAS_CPX")
            set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${CMAKE_CXX_LINK_FLAGS_CPLEX}")
            include_directories(${CPLEX_INCLUDES})
            target_link_libraries(${TARGET} ${CPLEX_LIBRARIES})
        endif()
    endif()

    if(OSI_Clp_FOUND)
        find_package(Clp)
        if (CLP_FOUND)
            add_definitions("-D COIN_HAS_CLP")
            include_directories(${CLP_INCLUDES})
            target_link_libraries(${TARGET} ${CLP_LIBRARIES})
        endif()
    endif()

    if(OSI_Grb_FOUND)
        message(FATAL_ERROR "Gurobi is not tested yet. Try to copy the config of cplex above and the file cmake/modules/FindCplex.cmake and adapt them for gurobi.")
        add_definitions("-D COIN_HAS_GRB")
    endif()
endif()
set(CMAKE_FIND_LIBRARY_SUFFIXES ${_CMAKE_FIND_LIBRARY_SUFFIXES})
