# Version 2.6 introduces functions.
cmake_minimum_required(VERSION 2.6)

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

include("FastDownwardOptions")
include("FastDownwardPlugins")


# Default to Release build, unless specified otherwise
if(NOT CMAKE_BUILD_TYPE)
    message("Defaulting to release build.")
    set(CMAKE_BUILD_TYPE "RELEASE")
endif(NOT CMAKE_BUILD_TYPE)

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

set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BITWIDTHOPT}")
# Note: we write "-std=c++0x" rather than "-std=c++11" to support gcc 4.4.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -pedantic -Wno-deprecated -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Iext")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    ## If you have Mac OS, you may be getting tree.hh from /opt/local/include.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/local/include/")
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")

## The following lines contain workarounds for bugs when
## cross-compiling to 64 bit on 32-bit systems using gcc 4.4 or gcc
## 4.5 in some Ubuntu releases. (We don't usually cross-compile to
## 64-bit, but in some cases we do; e.g. we did for the IPC.) See
## http://stackoverflow.com/questions/4643197/missing-include-bits-cconfig-h-when-cross-compiling-64-bit-program-on-32-bit.
if(CMAKE_COMPILER_IS_GNUCXX)
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
                    OUTPUT_VARIABLE GCC_VERSION)
    string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
    if (GCC_VERSION VERSION_LESS 4.6)
        list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
        list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)
        message(STATUS "Version ${GCC_MAJOR}.${GCC_MINOR} < 4.6")
        # If the original list wasn't part of the string the result will be two list items.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/include/c++/${GCC_MAJOR}.${GCC_MINOR}/i686-linux-gnu")
    endif(GCC_VERSION VERSION_LESS 4.6)
endif(CMAKE_COMPILER_IS_GNUCXX)

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
    endif(CMAKE_COMPILER_IS_GNUCXX)

    if(${LOWER_CASE_BUILD_TYPE} STREQUAL "release")
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
        else(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
            message(WARNING "Static release build is only tested on linux.")
        endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    endif(${LOWER_CASE_BUILD_TYPE} STREQUAL "release")
endif(DOWNWARD_LINK_RELEASE_STATICALLY)

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
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")


## == Finding (and if necessary, compiling) the LP libraries ==
if(DOWNWARD_USE_LP)
    find_package(OSI REQUIRED)
    find_package(Clp REQUIRED)
    find_package(CoinUtils REQUIRED)

    # We define here that the appropriate include paths are used.  Note, however, that we do not need
    # to define library paths as e.g. ${CLP_LIBRARIES} contains an absolute path to a CLP library.
    include_directories(${OSI_INCLUDES}/coin ${CLP_INCLUDES}/coin ${COINUTILS_INCLUDES}/coin)
    target_link_libraries(${TARGET}
        ${OSICLP_LIBRARIES} ${CLP_LIBRARIES} ${COINUTILS_LIBRARIES} ${OSI_LIBRARIES}
    )
endif(DOWNWARD_USE_LP)

